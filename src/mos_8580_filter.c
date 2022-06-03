/*
 * A-SID - C64 bandpass filter + LFO
 *
 * Copyright (C) 2022 Orastron srl unipersonale
 *
 * A-SID is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * A-SID is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *
 * File author: Stefano D'Angelo
 */

#include "mos_8580_filter.h"

#include "common.h"
#include "ormath.h"

struct _ordsp_mos_8580_filter {
	// Constants
	// (these should actually be static const in the global (local) scope,
	// but since we need to do some calculations we define them here to avoid potential race conditions)
	float Ve_k;
	
	// Coefficients
	float in_B0;
	float in_mA1;
	float out_B0;
	float out_mA1;
	float dc_B0;
	float dc_mA1;
	float B0_low;
	float k1_low;
	float pi_fs;

	float B0;
	float k1;
	float k2;
	float k;
	float Vhp_dVbp_xxz1;
	float Vhp_dVlp_xxz1;
	float Vhp_dVbypass;

	// Parameters
	float cutoff;
	float resonance;
	float volume;
	float bypass;
	float lp;
	float bp;
	float hp;
	int param_changed;

	// States
	float in_z1;
	float Vbp_z1;
	float dVbp_z1;
	float Vlp_z1;
	float dVlp_z1;
	float out_z1;
	float dc_z1;
};

static const float kin = 5.838280339378168e-1f;
static const float Vmin = -4.757f;
static const float Vmax = 4.243f;

ordsp_mos_8580_filter ordsp_mos_8580_filter_new() {
	ordsp_mos_8580_filter instance = (ordsp_mos_8580_filter)ORDSP_MALLOC(sizeof(struct _ordsp_mos_8580_filter));
	if (instance != NULL) {
		instance->Ve_k = 0.026f * ormath_omega_3log(159.6931258945051f);

		instance->cutoff = 1.f;
		instance->resonance = 0.f;
	}
	return instance;
}

void ordsp_mos_8580_filter_free(ordsp_mos_8580_filter instance) {
	ORDSP_FREE(instance);
}

void ordsp_mos_8580_filter_set_sample_rate(ordsp_mos_8580_filter instance, float sample_rate) {
	instance->in_B0 = sample_rate / (sample_rate + 13.55344121872543f);
	instance->in_mA1 = (sample_rate - 13.55344121872543f) / (sample_rate + 13.55344121872543f);

	instance->out_B0 = ormath_tanf_div_3(50e3f / sample_rate) / (1.f + ormath_tanf_div_3(50e3 / sample_rate));
	instance->out_mA1 = (1.f - ormath_tanf_div_3(50e3f / sample_rate)) / (1.f + ormath_tanf_div_3(50e3f / sample_rate));

	instance->dc_B0 = sample_rate / (sample_rate + 3.141592653589793f);
	instance->dc_mA1 = (sample_rate - 3.141592653589793f) / (sample_rate + 3.141592653589793f);

	instance->B0_low = sample_rate + sample_rate;
	instance->k1_low = 1.f / instance->B0_low;
	instance->pi_fs = 3.141592653589793f / sample_rate;
}

void ordsp_mos_8580_filter_reset(ordsp_mos_8580_filter instance) {
	instance->param_changed = ~0;

	instance->in_z1 = 0.f;
	instance->Vbp_z1 = 0.f;
	instance->dVbp_z1 = 0.f;
	instance->Vlp_z1 = 0.f;
	instance->dVlp_z1 = 0.f;
	instance->out_z1 = 0.f;
	instance->dc_z1 = 0.f;
}

#define PARAM_CUTOFF		1
#define PARAM_RESONANCE		(1<<1)

void ordsp_mos_8580_filter_process(ordsp_mos_8580_filter instance, const float* x, float* y, int n_samples) {
	if (instance->param_changed) {
		if (instance->param_changed & (PARAM_CUTOFF | PARAM_RESONANCE)) {
			if (instance->param_changed & PARAM_CUTOFF) {
				const float freq = 13164.18911276704f * instance->cutoff;
				if (freq >= 1.f) {
					instance->B0 = (6.283185307179586f * freq) / ormath_tanf_div_3(instance->pi_fs * freq);
					instance->k1 = 1.f / instance->B0;
				}
				else {
					instance->B0 = instance->B0_low;
					instance->k1 = instance->k1_low;
				}
				instance->k2 = 6.283185307179586f * freq;
			}
			if (instance->param_changed & PARAM_RESONANCE)
				instance->k = -1.254295325783559f + instance->resonance * (1.416499376036724f + instance->resonance * -0.5331537930049455f);

			const float Vhp_x1 = instance->k1 * (instance->k1 * instance->k2 - instance->k);
			const float Vhp_den = 1.f / (instance->k2 * Vhp_x1 + 1.f);

			instance->Vhp_dVbp_xxz1 = Vhp_den * Vhp_x1;
			instance->Vhp_dVlp_xxz1 = Vhp_den * -instance->k1;
			instance->Vhp_dVbypass = Vhp_den * -kin;
		}
	}
	const float kvol = -1.0435f * instance->volume;
	const float kbypass = -0.8653168127329506f * instance->bypass;
	
	for (int i = 0; i < n_samples; i++) {
		const float Vin = x[i];

		// input

		const float in_x1 = instance->in_B0 * Vin;
		const float Vbypass = in_x1 + instance->in_z1;
		instance->in_z1 = instance->in_mA1 * Vbypass - in_x1;

		// filter

		const float dVbp_xxz1 = instance->B0 * instance->Vbp_z1 + instance->dVbp_z1;
		const float dVlp_xxz1 = instance->B0 * instance->Vlp_z1 + instance->dVlp_z1;
		const float Vhp = instance->Vhp_dVbp_xxz1 * dVbp_xxz1 + instance->Vhp_dVlp_xxz1 * dVlp_xxz1 + instance->Vhp_dVbypass * Vbypass;
		const float Vbp = instance->k1 * (dVbp_xxz1 - instance->k2 * Vhp);
		const float Vlp = instance->k1 * (dVlp_xxz1 - instance->k2 * Vbp);
		const float dVbp = instance->B0 * Vbp - dVbp_xxz1;
		const float dVlp = instance->B0 * Vlp - dVlp_xxz1;

		instance->Vbp_z1 = Vbp;
		instance->dVbp_z1 = dVbp;
		instance->Vlp_z1 = Vlp;
		instance->dVlp_z1 = dVlp;

		// mix

		const float Vmix = ormath_clipf(-1.59074074074074f * (instance->hp * Vhp + instance->bp * Vbp + instance->lp * Vlp) + kbypass * Vbypass, Vmin, Vmax);

		// volume

		const float Vvol = ormath_clipf(kvol * Vmix, Vmin, Vmax);

		// out lowpass

		const float out_x1 = instance->out_B0 * Vvol;
		const float Vb = out_x1 + instance->out_z1;
		instance->out_z1 = out_x1 + instance->out_mA1 * Vb;

		// out buffer

		const float Ve = 0.026f * ormath_omega_3log(38.46153846153846f * Vb + 159.6931258945051f) - instance->Ve_k;

		// dc block

		const float dc_x1 = instance->dc_B0 * Ve;
		const float Vout = dc_x1 + instance->dc_z1;
		instance->dc_z1 = instance->dc_mA1 * Ve - dc_x1;

		y[i] = Vout;
	}
}

void ordsp_mos_8580_filter_set_cutoff(ordsp_mos_8580_filter instance, float value) {
	if (instance->cutoff != value) {
		instance->cutoff = value;
		instance->param_changed |= PARAM_CUTOFF;
	}
}

void ordsp_mos_8580_filter_set_resonance(ordsp_mos_8580_filter instance, float value) {
	if (instance->resonance != value) {
		instance->resonance = value;
		instance->param_changed |= PARAM_RESONANCE;
	}
}

void ordsp_mos_8580_filter_set_volume(ordsp_mos_8580_filter instance, float value) {
	instance->volume = value;
}

void ordsp_mos_8580_filter_set_mode(ordsp_mos_8580_filter instance, float bypass, float lp, float bp, float hp) {
	instance->bypass = bypass;
	instance->lp = lp;
	instance->bp = bp;
	instance->hp = hp;
}
