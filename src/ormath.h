/*
 * A-SID - C64 bandpass filter + LFO
 *
 * Copyright (C) 2021, 2022 Orastron srl unipersonale
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
 *
 * Part of the code in this file is derived from omega.h by Stefano D'Angelo,
 * which is released under the following conditions:
 *
 * Copyright (C) 2019 Stefano D'Angelo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Such part is itself based on the theory in
 *
 * S. D'Angelo, L. Gabrielli, and L. Turchet, "Fast Approximation of the
 * Lambert W Function for Virtual Analog Modeling", 22nd Intl. Conf. Digital
 * Audio Effects (DAFx-19), Birmingham, UK, September 2019.
 */

#ifndef _ORMATH_H
#define _ORMATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
	float f;
	int32_t i;
} ormath_floatint;

// I hope the target architecture and compiler will use conditional ops here

static inline int32_t ormath_signexti32(int32_t x) {
	return x < 0 ? ~0 : 0;
}

static inline int32_t ormath_mini32(int32_t a, int32_t b) {
	return a < b ? a : b;
}

static inline int32_t ormath_maxi32(int32_t a, int32_t b) {
	return a > b ? a : b;
}

static inline int32_t ormath_clipi32(int32_t x, int32_t m, int32_t M) {
	return x < m ? m : (x > M ? M : x);
}

// Here instead I don't trust C semantics to get close to conditional ops for
// floating point numbers

static inline float ormath_signf(float x) {
	ormath_floatint v = {.f = x};
	ormath_floatint one = {.f = 1.f};
	one.i |= v.i & 0x80000000;
	return one.f;
}

static inline float ormath_absf(float x) {
	ormath_floatint v = {.f = x};
	v.i = v.i & 0x7fffffff;
	return v.f;
}

static inline float ormath_min0xf(float x) {
	return 0.5f * (x - ormath_absf(x));
}

static inline float ormath_max0xf(float x) {
	return 0.5f * (x + ormath_absf(x));
}

static inline float ormath_minf(float a, float b) {
	return a + ormath_min0xf(b - a);
}

static inline float ormath_maxf(float a, float b) {
	return a + ormath_max0xf(b - a);
}

static inline float ormath_clipf(float x, float m, float M) {
	return ormath_minf(ormath_maxf(x, m), M);
}

static inline float ormath_truncf(float x) {
	ormath_floatint v = {.f = x};
	int32_t ex = (v.i & 0x7f800000) >> 23;
	int32_t m = (~0) << ormath_clipi32(150 - ex, 0, 23);
	m &= ormath_signexti32(126 - ex) | 0x80000000;
	v.i &= m;
	return v.f;
}

static inline float ormath_roundf(float x) {
	ormath_floatint v = {.f = x};
	int32_t ex = (v.i & 0x7f800000) >> 23;
	int32_t sh = ormath_clipi32(150 - ex, 0, 23);
	int32_t mt = (~0) << sh;
	mt &= ormath_signexti32(126 - ex) | 0x80000000;
	int32_t mr = (1 << sh) >> 1;
	mr &= ormath_signexti32(125 - ex);
	ormath_floatint s = {.f = ormath_signf(x)};
	int32_t ms = ormath_signexti32((v.i & mr) << (32 - sh));
	v.i &= mt;
	s.i &= ms;
	return v.f + s.f;
}

static inline float ormath_floorf(float x) {
	ormath_floatint t = {.f = ormath_truncf(x)}; // first bit set when t < 0
	ormath_floatint y = {.f = x - t.f}; // first bit set when t > x
	ormath_floatint s = {.f = 1.f};
	s.i &= ormath_signexti32(t.i & y.i);
	return t.f - s.f;
}

static inline float ormath_sinf_3(float x) {
	x = 0.1591549430918953f * x;
	x = x - ormath_floorf(x);
	float xp1 = x + x - 1.f;
	float xp2 = ormath_absf(xp1);
	float xp = 1.570796326794897f - 1.570796326794897f * ormath_absf(xp2 + xp2 - 1.f);
	return -ormath_signf(xp1) * (xp + xp * xp * (-0.05738534102710938f - 0.1107398163618408f * xp));
}

static inline float ormath_cosf_3(float x) {
	return ormath_sinf_3(x + 1.570796326794896f);
}

static inline float ormath_tanf_div_3(float x) {
	return ormath_sinf_3(x) / ormath_cosf_3(x);
}

static inline float ormath_log2f_3(float x) {
	ormath_floatint v = {.f = x};
	int ex = v.i & 0x7f800000;
	int e = (ex >> 23) - 127;
	v.i = (v.i - ex) | 0x3f800000;
	return (float)e - 2.213475204444817f + v.f * (3.148297929334117f + v.f * (-1.098865286222744f + v.f * 0.1640425613334452f));
}

static inline float ormath_logf_3(float x) {
	return 0.693147180559945f * ormath_log2f_3(x);
}

static inline float ormath_omega_3log(float x) {
	static const float x1 = -3.341459552768620f;
	static const float x2 = 8.f;
	static const float a = -1.314293149877800e-3f;
	static const float b = 4.775931364975583e-2f;
	static const float c = 3.631952663804445e-1f;
	static const float d = 6.313183464296682e-1f;
	x = ormath_maxf(x, x1);
	return x <= x2 ? d + x * (c + x * (b + x * a)) : x - ormath_logf_3(x);
}

#ifdef __cplusplus
}
#endif

#endif
