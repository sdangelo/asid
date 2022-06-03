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

#ifndef _ASID_H
#define _ASID_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _asid* asid;

asid asid_new();
void asid_free(asid instance);
void asid_set_sample_rate(asid instance, float sample_rate);
void asid_reset(asid instance);
void asid_process(asid instance, const float** x, float** y, int n_samples);
void asid_set_parameter(asid instance, int index, float value);
float asid_get_cutoff_modulated(asid instance);

#ifdef __cplusplus
}
#endif

#endif
