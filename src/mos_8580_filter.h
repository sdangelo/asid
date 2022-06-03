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

#ifndef _ORDSP_MOS_8580_FILTER_H
#define _ORDSP_MOS_8580_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ordsp_mos_8580_filter* ordsp_mos_8580_filter;

ordsp_mos_8580_filter ordsp_mos_8580_filter_new();
void ordsp_mos_8580_filter_free(ordsp_mos_8580_filter instance);
void ordsp_mos_8580_filter_set_sample_rate(ordsp_mos_8580_filter instance, float sample_rate);
void ordsp_mos_8580_filter_reset(ordsp_mos_8580_filter instance);
void ordsp_mos_8580_filter_process(ordsp_mos_8580_filter instance, const float* x, float* y, int n_samples);
void ordsp_mos_8580_filter_set_cutoff(ordsp_mos_8580_filter instance, float value);		// value in [0, 1], corresponds to original range [0, 2047]
void ordsp_mos_8580_filter_set_resonance(ordsp_mos_8580_filter instance, float value);	// value in [0, 1], corresponds to original range [0, 15]
void ordsp_mos_8580_filter_set_volume(ordsp_mos_8580_filter instance, float value);		// value in [0, 1], corresponds to original range [0, 15]
void ordsp_mos_8580_filter_set_mode(ordsp_mos_8580_filter instance, float bypass, float lp, float bp, float hp);	// values 0, 1 correspond to originals (either 0 or 1)

#ifdef __cplusplus
}
#endif

#endif
