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
 * File author: Paolo Marrone, Stefano D'Angelo
 */

#include "asid_gui.h"

#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "screens.h"

#include <stdio.h>

#define BILINEAR_INTERPOLATION

static const uint32_t width_default = 404;
static const uint32_t height_default = 284;

static const uint32_t inner_width_default = 320;
static const uint32_t inner_height_default = 200;
static const uint32_t inner_size_pixel = inner_width_default * inner_height_default * 4;

static const float imgrate = 1.422535211267606f;
static const float innerImgrate = 1.6f;

static const uint32_t padding_default = 42;

// Default slider positions
static const uint32_t param_cutoff_x1 = 22; // left border
static const uint32_t param_cutoff_x2 = 33; // right border
static const uint32_t param_lfoamt_x1 = 94;
static const uint32_t param_lfoamt_x2 = 105;
static const uint32_t param_lfospd_x1 = 166;
static const uint32_t param_lfospd_x2 = 177;
static const uint32_t params_y1 = 38;   // top border
static const uint32_t params_y2 = 161;  // bottom border
static const uint32_t params_w = 10;    // without borders
static const uint32_t params_h = 122;   // without borders
static const uint32_t params_inner_h = 121; // Just the content
static const uint32_t params_inner_y = 40;

// Default parameter BOX positions
static const uint32_t parambox_cutoff_x1 = 0;
static const uint32_t parambox_cutoff_x2 = 57;
static const uint32_t parambox_lfoamt_x1 = 72;
static const uint32_t parambox_lfoamt_x2 = 129;
static const uint32_t parambox_lfospd_x1 = 144;
static const uint32_t parambox_lfospd_x2 = 201;
static const uint32_t paramboxs_y1 = 38;
static const uint32_t paramboxs_y2 = 187;
static const uint32_t paramboxs_w = 58;
static const uint32_t paramboxs_h = 152;

// Default modulated cutoff slider position
static const uint32_t modCutoff_x1 = 35;
static const uint32_t modCutoff_x2 = 39;
static const uint32_t modCutoff_y1 = 40;
static const uint32_t modCutoff_y2 = 159;
static const uint32_t modCutoff_w  = 5;
static const uint32_t modCutoff_h  = 120;

static uint8_t colors_default[4][3] = {
	{102, 204, 0},
	{170, 0, 0},
	{204, 68, 204},
	{0, 0, 136}
};

static uint8_t color_grey[3] = {51, 51, 51};

static const uint8_t* screen_map_defaults[] = {screen1_map_default, screen2_map_default, screen3_map_default, screen4_map_default};

struct _asid_gui {
	gui	 	 g;
	asid_gui_view	 views;
	void		*data;

	float (*get_parameter)(asid_gui gui, uint32_t id);
	void (*set_parameter)(asid_gui gui, uint32_t id, float value);
};

struct _asid_gui_view {
	asid_gui	 gui;
	asid_gui_view	 next;
	window		 win;

	unsigned char* screen_maps[4];

	unsigned char* resized[4];

	float scaleFactor;
	float scaleFactorInv;

	uint32_t w;
	uint32_t h;

	uint32_t xWhite;
	uint32_t yWhite;
	uint32_t wWhite;
	uint32_t hWhite;

	uint32_t xContent;
	uint32_t yContent;
	uint32_t wContent;
	uint32_t hContent;

	uint32_t xCutoff;
	uint32_t xAmount;
	uint32_t xSpeed;
	uint32_t yParams;
	uint32_t wParams;
	uint32_t hParams;

	uint32_t xBoxCutoff;
	uint32_t xBoxAmount;
	uint32_t xBoxSpeed;
	uint32_t yBoxParams;
	uint32_t wBoxParams;
	uint32_t hBoxParams;

	uint32_t xModCutoff;
	uint32_t yModCutoff;
	uint32_t wModCutoff;
	uint32_t hModCutoff;

	// Params state
	float paramValues[3];
	float paramMappedValues[3];

	// Hover state
	char param_hover; // -1 = None, 0 = cutoff, 1 = amount, 2 = speed

	// Mouse selection state
	char param_selected; // -1 = None, 0 = cutoff, 1 = amount, 2 = speed
	int mouse_old_y;

	// Screen map default
	char screen_map_selected_old;
	char screen_map_selected;

	// Mod cutoff
	float modCutoffValue;

	// Redrawing state
	char paramToRedraw[3];
	char modCutoffToRedraw;
	char toResize;
};

static uint32_t floorI(float x) {
	return (uint32_t) x;
}

static uint32_t ceilI(float x) {
	int32_t f = floorI(x);
	return f + (x - (float) f <= 0.000001f ? 0 : 1);
}

static void floorceilI(float x, uint32_t* floor, uint32_t* ceil) {
	*floor = (uint32_t) x;
	*ceil  = *floor + (x - (float) *floor <= 0.000001f ? 0 : 1);
}

static uint32_t minI(uint32_t x1, uint32_t x2) {
	return x1 <= x2 ? x1 : x2;
}

static float minF(float x1, float x2) {
	return x1 < x2 ? x1 : x2;
}

static float maxF(float x1, float x2) {
	return x1 > x2 ? x1 : x2;
}

static float clipF(float x, float min, float max) {
	return minF(max, maxF(min, x));
}

static void interpolate (
	unsigned char *src, unsigned char *dest, 
	uint32_t src_w, uint32_t src_h,
	uint32_t src_target_x, uint32_t src_target_y, uint32_t src_target_w, uint32_t src_target_h,
	uint32_t dest_w, uint32_t dest_h,
	uint32_t dest_target_x, uint32_t dest_target_y, uint32_t dest_target_w, uint32_t dest_target_h)
{
#ifdef BILINEAR_INTERPOLATION
	const float w_scale_factor = (float) src_target_w / (float) dest_target_w;
	const float h_scale_factor = (float) src_target_h / (float) dest_target_h;

	const uint32_t wOffset = (dest_w - dest_target_w) << 2;
	uint32_t newIndex = (dest_target_y * dest_w + dest_target_x) << 2;

	for (uint32_t ri = 0; ri < dest_target_h; ri++) {
		const float y = (float) ri * h_scale_factor + src_target_y;
		uint32_t y_floor, y_ceil;
		floorceilI(y, &y_floor, &y_ceil);
		y_ceil = minI(src_h - 1, y_ceil);
		const int B = (int) ((y - (float) y_floor) * 255.f);

		for (uint32_t ci = 0; ci < dest_target_w; ci++) {

			const float x = (float) ci * w_scale_factor + src_target_x;
			uint32_t x_floor, x_ceil;
			floorceilI(x, &x_floor, &x_ceil);
			x_ceil = minI(src_w - 1, x_ceil);
			
			unsigned char a, b, c;

			if (x_ceil == x_floor && y_ceil == y_floor) {
				const uint32_t pixelIndex = (y_floor * src_w + x_floor) << 2;
				a = src[pixelIndex + 0];
				b = src[pixelIndex + 1];
				c = src[pixelIndex + 2];
			}

			else if (x_ceil == x_floor) {
				const uint32_t pixelIndex1 = (y_floor * src_w + x_floor) << 2;
				const uint32_t pixelIndex2 = pixelIndex1 + (src_w << 2);

				a = (unsigned char) ((int) src[pixelIndex1 + 0] + ((B * ((int) src[pixelIndex2 + 0] - (int) src[pixelIndex1 + 0])) >> 8));
				b = (unsigned char) ((int) src[pixelIndex1 + 1] + ((B * ((int) src[pixelIndex2 + 1] - (int) src[pixelIndex1 + 1])) >> 8));
				c = (unsigned char) ((int) src[pixelIndex1 + 2] + ((B * ((int) src[pixelIndex2 + 2] - (int) src[pixelIndex1 + 2])) >> 8));
			}

			else if (y_ceil == y_floor) {
				const uint32_t pixelIndex1 = (y_floor * src_w + x_floor) << 2;
				const uint32_t pixelIndex2 = pixelIndex1 + 4;

				const int A = (int) ((x - (float) x_floor) * 255.f);

				a = (unsigned char) ((int) src[pixelIndex1 + 0] + ((A * ((int) src[pixelIndex2 + 0] - (int) src[pixelIndex1 + 0])) >> 8));
				b = (unsigned char) ((int) src[pixelIndex1 + 1] + ((A * ((int) src[pixelIndex2 + 1] - (int) src[pixelIndex1 + 1])) >> 8));
				c = (unsigned char) ((int) src[pixelIndex1 + 2] + ((A * ((int) src[pixelIndex2 + 2] - (int) src[pixelIndex1 + 2])) >> 8));
			}

			else {
				const uint32_t pixelIndex1 = (y_floor * src_w + x_floor) << 2;
				const uint32_t pixelIndex2 = pixelIndex1 + 4;
				const uint32_t pixelIndex3 = pixelIndex1 + (src_w << 2);
				const uint32_t pixelIndex4 = pixelIndex3 + 4;

				const int A = (int) ((x - (float) x_floor) * 255.f);
				
				const int a1 = (int) src[pixelIndex1 + 0] + ((A * ((int) src[pixelIndex2 + 0] - (int) src[pixelIndex1 + 0])) >> 8);
				const int b1 = (int) src[pixelIndex1 + 1] + ((A * ((int) src[pixelIndex2 + 1] - (int) src[pixelIndex1 + 1])) >> 8);
				const int c1 = (int) src[pixelIndex1 + 2] + ((A * ((int) src[pixelIndex2 + 2] - (int) src[pixelIndex1 + 2])) >> 8);

				const int a2 = (int) src[pixelIndex3 + 0] + ((A * ((int) src[pixelIndex4 + 0] - (int) src[pixelIndex3 + 0])) >> 8);
				const int b2 = (int) src[pixelIndex3 + 1] + ((A * ((int) src[pixelIndex4 + 1] - (int) src[pixelIndex3 + 1])) >> 8);
				const int c2 = (int) src[pixelIndex3 + 2] + ((A * ((int) src[pixelIndex4 + 2] - (int) src[pixelIndex3 + 2])) >> 8);

				a = (unsigned char) (a1 + ((B * (a2 - a1)) >> 8));
				b = (unsigned char) (b1 + ((B * (b2 - b1)) >> 8));
				c = (unsigned char) (c1 + ((B * (c2 - c1)) >> 8));
			}

			dest[newIndex++] = a;
			dest[newIndex++] = b;
			dest[newIndex++] = c;
			dest[newIndex++] = 0;
		}
		newIndex += wOffset;
	}
#else
	const float w_scale_factor = (float) src_target_w / (float) dest_target_w;
	const float h_scale_factor = (float) src_target_h / (float) dest_target_h;

	const uint32_t wOffset = (dest_w - dest_target_w) << 2;
	uint32_t newIndex = (dest_target_y * dest_w + dest_target_x) << 2;

	for (uint32_t ri = 0; ri < dest_target_h; ri++) {
		const float y = (float) ri * h_scale_factor + src_target_y;
		uint32_t y_floor, y_ceil;
		floorceilI(y, &y_floor, &y_ceil);
		y_ceil = minI(src_h - 1, y_ceil);

		const uint32_t yFall = y - y_floor < 0.5f ? y_floor : y_ceil;
		
		for (uint32_t ci = 0; ci < dest_target_w; ci++) {

			const float x = (float) ci * w_scale_factor + src_target_x;
			uint32_t x_floor, x_ceil;
			floorceilI(x, &x_floor, &x_ceil);
			x_ceil = minI(src_w - 1, x_ceil);
			
			const uint32_t xFall = x - x_floor < 0.5f ? x_floor : x_ceil;

			const uint32_t pixelIndex = (yFall * src_w + xFall) << 2;

			dest[newIndex++] = src[pixelIndex + 0];;
			dest[newIndex++] = src[pixelIndex + 1];;
			dest[newIndex++] = src[pixelIndex + 2];;
			dest[newIndex++] = 0;
		}
		newIndex += wOffset;
	}
#endif
}

static void draw_parameter_fixed(asid_gui_view view, char p) {
	uint32_t pb_x1, pb_x2, yValue;
	uint32_t p_x1, p_x2;

	if (p == 0) {
		pb_x1 = parambox_cutoff_x1;
		pb_x2 = parambox_cutoff_x2;
		p_x1 = param_cutoff_x1;
		p_x2 = param_cutoff_x2;
		yValue = (uint32_t) ((1.f - view->paramMappedValues[p]) * (float) params_inner_h) + params_inner_y;
	}
	else if (p == 1) {
		pb_x1 = parambox_lfoamt_x1;
		pb_x2 = parambox_lfoamt_x2;
		p_x1 = param_lfoamt_x1;
		p_x2 = param_lfoamt_x2;
		yValue = (uint32_t) ((1.f - view->paramMappedValues[p]) * (float) params_inner_h) + params_inner_y;
	}
	else if (p == 2) {
		pb_x1 = parambox_lfospd_x1;
		pb_x2 = parambox_lfospd_x2;
		p_x1 = param_lfospd_x1;
		p_x2 = param_lfospd_x2;
		yValue = (uint32_t) ((1.f - view->paramMappedValues[p]) * (float) params_inner_h) + params_inner_y;
	}
	else
		return;

	uint32_t index = ((paramboxs_y1) * inner_width_default + pb_x1) << 2;
	uint32_t wOffset = (inner_width_default - paramboxs_w) << 2;

	if (view->param_hover == p) { // isHover
		for (uint32_t c = paramboxs_y1; c <= paramboxs_y2; c++) {
			for (int r = pb_x1; r <= pb_x2; r++) {
				if (screen_map_defaults[0][index] != 255) {
					for (char i = 0; i < 4; i++) {
						view->screen_maps[i][index + 0] = colors_default[i][0];
						view->screen_maps[i][index + 1] = colors_default[i][1];
						view->screen_maps[i][index + 2] = colors_default[i][2];
					}
				}
				index += 4;
			}
			index += wOffset;
		}
	}
	else {
		for (uint32_t c = paramboxs_y1; c <= paramboxs_y2; c++) {
			for (int r = pb_x1; r <= pb_x2; r++) {
				if (screen_map_defaults[0][index] != 255) {
					for (char i = 0; i < 4; i++) {
						view->screen_maps[i][index + 0] = screen_map_defaults[0][index + 0];
						view->screen_maps[i][index + 1] = screen_map_defaults[0][index + 1];
						view->screen_maps[i][index + 2] = screen_map_defaults[0][index + 2];
					}
				}
				index += 4;
			}
			index += wOffset;
		}
	}
	index = (params_inner_y * inner_width_default + p_x1 + 1) << 2;
	wOffset = (inner_width_default - params_w) << 2;
	for (uint32_t c = params_inner_y; c < yValue; c++) {
		for (int r = p_x1 + 1; r < p_x2; r++) {
			for (char i = 0; i < 4; i++) {
				view->screen_maps[i][index + 0] = 255;
				view->screen_maps[i][index + 1] = 255;
				view->screen_maps[i][index + 2] = 255;
			}
			index += 4;
		}
		index += wOffset;
	}
}

static void draw_parameter_resized(asid_gui_view view, char p) {
	uint32_t pb_x1, resized_x1;
	if (p == 0) {
		pb_x1 = parambox_cutoff_x1;
		resized_x1 = view->xBoxCutoff;
	}
	else if (p == 1) {
		pb_x1 = parambox_lfoamt_x1;
		resized_x1 = view->xBoxAmount;
	}
	else if (p == 2) {
		pb_x1 = parambox_lfospd_x1;
		resized_x1 = view->xBoxSpeed;
	}
	else
		return;

	for (int ri = 0; ri < 4; ri++) {
		interpolate(
			view->screen_maps[ri], view->resized[ri], 
			inner_width_default, inner_height_default,  
			pb_x1, paramboxs_y1, paramboxs_w, paramboxs_h,
			view->w, view->h,
			resized_x1, view->yBoxParams, view->wBoxParams, view->hBoxParams);
	}
}

static void update_view_parameter(asid_gui_view view, char p) {
	uint32_t resized_x1;
	if (p == 0) {
		resized_x1 = view->xBoxCutoff;
	}
	else if (p == 1) {
		resized_x1 = view->xBoxAmount;
	}
	else if (p == 2) {
		resized_x1 = view->xBoxSpeed;
	}
	else
		return;

	gui_window_draw (view->win, view->resized[view->screen_map_selected], 
		resized_x1, view->yBoxParams, view->w, view->h, 
		resized_x1, view->yBoxParams, view->wBoxParams, view->hBoxParams);
}

static void draw_modCutoff_slider_fixed (asid_gui_view view) {
	static const uint32_t indexStart = (modCutoff_y1 * inner_width_default + modCutoff_x1) << 2;
	static const uint32_t wOffset = (inner_width_default - modCutoff_w) << 2; 
	const uint32_t yMid = modCutoff_y1 + (uint32_t) ((1.f - view->modCutoffValue) * modCutoff_h);
	uint8_t* colors = color_grey;
	for (int ri = 0; ri < 4; ri++) {
		if (view->param_selected == 0 || view->param_hover == 0)
			colors = colors_default[ri];
		uint32_t index = indexStart;
		uint32_t r = modCutoff_y1;
		for (; r < yMid; r++) {
			for (uint32_t c = modCutoff_x1; c <= modCutoff_x2; c++) {
				view->screen_maps[ri][index + 0] = 255; // Optimize: see array of char as array of int32.
				view->screen_maps[ri][index + 1] = 255;
				view->screen_maps[ri][index + 2] = 255;
				index += 4;
			}
			index += wOffset;
		}
		for (; r <= modCutoff_y2; r++) {
			for (uint32_t c = modCutoff_x1; c <= modCutoff_x2; c++) {
				view->screen_maps[ri][index + 0] = colors[0];
				view->screen_maps[ri][index + 1] = colors[1];
				view->screen_maps[ri][index + 2] = colors[2];
				index += 4;
			}
			index += wOffset;
		}
	}
}

static void draw_padding (unsigned char* img, uint32_t xBlack, uint32_t yBlack, uint32_t wBlack, uint32_t hBlack,
	uint32_t xWhite, uint32_t yWhite, uint32_t wWhite, uint32_t hWhite) {
	memset(img, 0, (wBlack * hBlack) << 2);

	uint32_t index = (yWhite * wBlack + xWhite) << 2;
	for (uint32_t i = 0; i < hWhite; i++) {
		memset(&(img[index]), 255, wWhite << 2);
		index += wBlack << 2;
	}
}

static void draw_resized (asid_gui_view view) {
	const uint32_t size = (view->w * view->h) << 2;
	view->resized[0] = (unsigned char*) realloc(view->resized[0], size);
	view->resized[1] = (unsigned char*) realloc(view->resized[1], size);
	view->resized[2] = (unsigned char*) realloc(view->resized[2], size);
	view->resized[3] = (unsigned char*) realloc(view->resized[3], size);

	if (view->resized[0] == NULL || view->resized[1] == NULL || view->resized[2] == NULL || view->resized[3] == NULL) {
		return;
	}

	for (int ri = 0; ri < 4; ri++) {
		draw_padding(view->resized[ri], 0, 0, view->w, view->h, view->xWhite, view->yWhite, view->wWhite, view->hWhite); 
		interpolate(
		  	view->screen_maps[ri], view->resized[ri], 
		  	inner_width_default, inner_height_default,  
		  	0, 0, inner_width_default, inner_height_default,
		  	view->w, view->h,
		  	view->xContent, view->yContent, view->wContent, view->hContent);
	}
}

static void resize(asid_gui_view view, uint32_t width, uint32_t height) {
	float newrate = (float)width / (float) height;

	if (newrate >= imgrate) {
		view->wWhite = (uint32_t) (imgrate * (float) height);
		view->hWhite = height;
		view->xWhite = (width - view->wWhite) >> 1;
		view->yWhite = 0;
	}
	else {
		view->wWhite = width;
		view->hWhite = (uint32_t) (((float) width) / imgrate);
		view->xWhite = 0;
		view->yWhite = (height - view->hWhite) >> 1;
	}

	view->scaleFactor = (float) view->wWhite / (float) width_default;
	view->scaleFactorInv = 1.f / view->scaleFactor;
	uint32_t newPadding = (uint32_t) (((float) padding_default) * view->scaleFactor);

	view->xContent = view->xWhite + newPadding;
	view->yContent = view->yWhite + newPadding;
	view->wContent = view->wWhite - (newPadding << 1);
	view->hContent = view->hWhite - (newPadding << 1);

	view->xCutoff = (uint32_t) (view->xContent + ((float) (param_cutoff_x1 + 1)) * view->scaleFactor);
	view->xAmount = (uint32_t) (view->xContent + ((float) (param_lfoamt_x1 + 1)) * view->scaleFactor);
	view->xSpeed =  (uint32_t) (view->xContent + ((float) (param_lfospd_x1 + 1)) * view->scaleFactor);
	view->yParams = (uint32_t) (view->yContent + ((float) (params_y1 + 1)) * view->scaleFactor);
	view->wParams = (uint32_t) (((float) params_w) * view->scaleFactor);
	view->hParams = (uint32_t) (((float) params_h) * view->scaleFactor);

	view->xBoxCutoff = (uint32_t) (view->xContent + ((float) (parambox_cutoff_x1)) * view->scaleFactor); // Forse round è meglio
	view->xBoxAmount = (uint32_t) (view->xContent + ((float) (parambox_lfoamt_x1)) * view->scaleFactor);
	view->xBoxSpeed =  (uint32_t) (view->xContent + ((float) (parambox_lfospd_x1)) * view->scaleFactor);
	view->yBoxParams = (uint32_t) (view->yContent + ((float) (paramboxs_y1)) * view->scaleFactor);
	view->wBoxParams = ceilI (((float) paramboxs_w) * view->scaleFactor);
	view->hBoxParams = ceilI (((float) paramboxs_h) * view->scaleFactor);

	view->xModCutoff = view->xContent + (uint32_t) (((float) modCutoff_x1) * view->scaleFactor);
	view->yModCutoff = view->yContent + (uint32_t) (((float) modCutoff_y1) * view->scaleFactor);
	view->wModCutoff = ceilI (((float) modCutoff_w) * view->scaleFactor);
	view->hModCutoff = ceilI (((float) modCutoff_h) * view->scaleFactor);

	view->w = width;
	view->h = height;

	view->toResize = 1;
}

static void draw(asid_gui_view view) {
	for (int i = 0; i < 3; i++) {
		if (view->paramToRedraw[i])
			draw_parameter_fixed(view, i);
	}
	if (view->modCutoffToRedraw)
		draw_modCutoff_slider_fixed(view);

	if (view->toResize) {
		draw_resized(view);
		gui_window_draw (view->win, view->resized[view->screen_map_selected], 0, 0, view->w, view->h, 0, 0, view->w, view->h);
		for (int i = 0; i < 3; i++) 
			view->paramToRedraw[i] = 1;
		view->modCutoffToRedraw = 0;
		view->toResize = 0;
	}
	else {
		for (int i = 0; i < 3; i++) {
			if (view->paramToRedraw[i])
				draw_parameter_resized(view, i);
		}
		if (view->paramToRedraw[0] == 0 && view->modCutoffToRedraw)
			draw_parameter_resized(view, 0);

		if (view->screen_map_selected != view->screen_map_selected_old) {
			gui_window_draw (view->win, view->resized[view->screen_map_selected], 
				view->xContent, view->yContent, view->w, view->h, 
				view->xContent, view->yContent, view->wContent, view->hContent);
			view->screen_map_selected_old = view->screen_map_selected;
		}
		else {
			for (int i = 0; i < 3; i++) {
				if (view->paramToRedraw[i])
					update_view_parameter(view, i);
			}
			if (view->paramToRedraw[0] == 0 && view->modCutoffToRedraw)
				update_view_parameter(view, 0);
		}

		view->paramToRedraw[0] = 0;
		view->paramToRedraw[1] = 0;
		view->paramToRedraw[2] = 0;
		view->modCutoffToRedraw = 0;
	}
}

static void on_resize(window w, uint32_t width, uint32_t height) {
	asid_gui_view view = (asid_gui_view)gui_window_get_data(w);
	resize(view, width, height);
	draw(view);
}

static void on_mouse_press (window w, int32_t x, int32_t y) {
	asid_gui_view view = (asid_gui_view)gui_window_get_data(w);

	x = (int) (((float) (x - (int) view->xContent)) * view->scaleFactorInv);
	y = (int) (((float) (y - (int) view->yContent)) * view->scaleFactorInv);

	if (y >= paramboxs_y1 && y <= paramboxs_y2) {
		if (x >= parambox_cutoff_x1 && x <= parambox_cutoff_x2) {
			view->param_selected = 0;
		}
		else if (x >= parambox_lfoamt_x1 && x <= parambox_lfoamt_x2) {
			view->param_selected = 1;
		}
		else if (x >= parambox_lfospd_x1 && x <= parambox_lfospd_x2) {
			view->param_selected = 2;
		}
		else {
			view->param_selected = -1;
		}
		view->mouse_old_y = y;
	}
}

static void on_mouse_release (window w, int32_t x, int32_t y) {
	asid_gui_view view = (asid_gui_view)gui_window_get_data(w);
	view->param_selected = -1;
	//view->mouse_old_y = 0; // Maybe something better
}

static void on_mouse_move (window w, int32_t x, int32_t y, uint32_t mouseState) {
	asid_gui_view view = (asid_gui_view)gui_window_get_data(w);

	x = (int) (((float) (x - (int) view->xContent)) * view->scaleFactorInv);
	y = (int) (((float) (y - (int) view->yContent)) * view->scaleFactorInv);

	// Hover
	if (view->param_selected == -1) {
		char old_param_hover = view->param_hover;
		if (y >= paramboxs_y1 && y <= paramboxs_y2) {
			if (x >= parambox_cutoff_x1 && x <= parambox_cutoff_x2)
				view->param_hover = 0;
			else if (x >= parambox_lfoamt_x1 && x <= parambox_lfoamt_x2)
				view->param_hover = 1; 
			else if (x >= parambox_lfospd_x1 && x <= parambox_lfospd_x2)
				view->param_hover = 2;
			else
				view->param_hover = -1;
		}
		else
			view->param_hover = -1;

		if (old_param_hover == view->param_hover)
			;
		else {
			if (old_param_hover != -1)
				view->paramToRedraw[old_param_hover] = 1;
			if (view->param_hover != -1)
				view->paramToRedraw[view->param_hover] = 1;
			if (view->paramToRedraw[0])
				view->modCutoffToRedraw = 1;
		}
	}
	// Mouse diff
	if (view->param_selected != -1 && mouseState > 0) {
		float diff = ((float) (view->mouse_old_y - y)) / (float) params_h;
		if (diff != 0.f) {
			view->paramValues[view->param_selected] = clipF(view->paramValues[view->param_selected] + diff, 0.f, 1.f);
			float newMappedV = floorI(view->paramValues[view->param_selected] * 15.f) * 0.0666666666666666f;
			if (view->paramMappedValues[view->param_selected] != newMappedV) {
				view->paramMappedValues[view->param_selected] = newMappedV;
				view->paramToRedraw[view->param_selected] = -1;
				view->gui->set_parameter(view->gui, view->param_selected, view->paramMappedValues[view->param_selected]);
			}
		}
	}
	view->mouse_old_y = y;
}

static void view_on_param_set(asid_gui_view view, uint32_t id, float value) {
	if (id == 3) {
		view->modCutoffValue = value;
		char screen_map_selected_new;
		if (value < 0.25f) {
			screen_map_selected_new = 0;
		}
		else if (value < 0.5f) {
			screen_map_selected_new = 1;
		}
		else if (value < 0.75f) {
			screen_map_selected_new = 2;
		}
		else {
			screen_map_selected_new = 3;
		}

		view->screen_map_selected = screen_map_selected_new;
		view->modCutoffToRedraw = 1;
	}

	else {
		if (view->paramToRedraw[id] == -1) {// Set by the gui
			view->paramToRedraw[id] = 1;
			return;
		}
		view->paramValues[id] = clipF(value, 0.f, 1.f);
		float newMappedV = floorI(view->paramValues[id] * 15.f) * 0.0666666666666666f;
		if (view->paramMappedValues[id] != newMappedV) {
			view->paramMappedValues[id] = newMappedV;
			view->paramToRedraw[id] = 1;
		}
	}
}

asid_gui asid_gui_new(
	float (*get_parameter)(asid_gui gui, uint32_t id),
	void (*set_parameter)(asid_gui gui, uint32_t id, float value),
	void *data
) {
	asid_gui ret = (asid_gui)malloc(sizeof(struct _asid_gui));
	if (!ret)
		return NULL;

	ret->g = gui_new();
	if (ret->g == NULL) {
		free(ret);
		return NULL;
	}

	ret->get_parameter = get_parameter;
	ret->set_parameter = set_parameter;
	ret->data = data;
	ret->views = NULL;

	return ret;
}

void asid_gui_free(asid_gui gui) {
	gui_free(gui->g);
	free(gui);
}

void asid_gui_process_events(asid_gui gui) {
	gui_run(gui->g, 1);
}

uint32_t asid_gui_get_default_width(asid_gui gui) {
	return width_default;
}

uint32_t asid_gui_get_default_height(asid_gui gui) {
	return height_default;
}

void *asid_gui_get_data(asid_gui gui) {
	return gui->data;
}

void asid_gui_on_param_set(asid_gui gui, uint32_t id, float value) {
	for (asid_gui_view view = gui->views; view; view = view->next)
		view_on_param_set(view, id, value);
}

asid_gui_view asid_gui_view_new(asid_gui gui, void *parent) {
	asid_gui_view ret = (asid_gui_view)malloc(sizeof(struct _asid_gui_view));
	if (ret == NULL)
		return NULL;

	ret->gui = gui;

	ret->screen_maps[0] = (unsigned char*) malloc(inner_size_pixel);
	ret->screen_maps[1] = (unsigned char*) malloc(inner_size_pixel);
	ret->screen_maps[2] = (unsigned char*) malloc(inner_size_pixel);
	ret->screen_maps[3] = (unsigned char*) malloc(inner_size_pixel);

	if (!ret->screen_maps[0] || !ret->screen_maps[1] || !ret->screen_maps[2] || !ret->screen_maps[3]) {
		for (int i = 0; i < 4; i++)
			if (ret->screen_maps[i])
				free(ret->screen_maps[i]);
		return NULL;
	}

	memcpy(ret->screen_maps[0], screen_map_defaults[0], inner_size_pixel);
	memcpy(ret->screen_maps[1], screen_map_defaults[1], inner_size_pixel);
	memcpy(ret->screen_maps[2], screen_map_defaults[2], inner_size_pixel);
	memcpy(ret->screen_maps[3], screen_map_defaults[3], inner_size_pixel);

	ret->resized[0] = NULL;
	ret->resized[1] = NULL;
	ret->resized[2] = NULL;
	ret->resized[3] = NULL;

	ret->scaleFactor = 1.f;
	ret->scaleFactorInv = 1.f;

	ret->w = width_default;
	ret->h = height_default;

	ret->xWhite = 0;
	ret->yWhite = 0;
	ret->wWhite = width_default;
	ret->hWhite = height_default;

	ret->xContent = padding_default;
	ret->yContent = padding_default;
	ret->wContent = inner_width_default;
	ret->hContent = inner_height_default;

	ret->xCutoff = param_cutoff_x1;
	ret->xAmount = param_lfoamt_x1;
	ret->xSpeed  = param_lfospd_x1;
	ret->yParams = params_y1;
	ret->wParams = params_w;
	ret->hParams = params_h;

	ret->xBoxCutoff = parambox_cutoff_x1;
	ret->xBoxAmount = parambox_lfoamt_x1;
	ret->xBoxSpeed  = parambox_lfospd_x1;
	ret->yBoxParams = paramboxs_y1;
	ret->wBoxParams = paramboxs_w;
	ret->hBoxParams = paramboxs_h;

	ret->xModCutoff = modCutoff_x1;
	ret->yModCutoff = modCutoff_y1;
	ret->wModCutoff = modCutoff_w;
	ret->hModCutoff = modCutoff_h;

	ret->paramValues[0] = gui->get_parameter(gui, 0);
	ret->paramValues[1] = gui->get_parameter(gui, 1);
	ret->paramValues[2] = gui->get_parameter(gui, 2);

	ret->paramMappedValues[0] = floorI(ret->paramValues[0] * 15.f) * 0.0666666666666666f;
	ret->paramMappedValues[1] = floorI(ret->paramValues[1] * 15.f) * 0.0666666666666666f;
	ret->paramMappedValues[2] = floorI(ret->paramValues[2] * 15.f) * 0.0666666666666666f;

	ret->param_hover = -1; // -1 = None, 0 = cutoff, 1 = amount, 2 = speed
	ret->param_selected = -1; // -1 = None, 0 = cutoff, 1 = amount, 2 = speed
	ret->mouse_old_y = 0;

	ret->screen_map_selected = 3;

	ret->modCutoffValue = 0.f;

	// Redrawing state
	ret->paramToRedraw[0] = 0;
	ret->paramToRedraw[1] = 0;
	ret->paramToRedraw[2] = 0;
	ret->modCutoffToRedraw = 0;
	ret->toResize = 0;

	ret->win = gui_window_new(gui->g, parent, width_default, height_default);
	if (ret->win == NULL) {
		for (int i = 0; i < 4; i++)
			free(ret->screen_maps[i]);
		free(ret);
		return NULL;
	}

	ret->next = NULL;
	if (gui->views == NULL)
		gui->views = ret;
	else {
		asid_gui_view n = gui->views;
		while (n->next)
			n = n->next;
		n->next = ret;
	}
	
	gui_window_set_data(ret->win, (void *)ret);
	gui_window_set_cb(ret->win, GUI_CB_RESIZE, (gui_cb)on_resize);
	gui_window_set_cb(ret->win, GUI_CB_MOUSE_PRESS, (gui_cb)on_mouse_press);
	gui_window_set_cb(ret->win, GUI_CB_MOUSE_RELEASE, (gui_cb)on_mouse_release);
	gui_window_set_cb(ret->win, GUI_CB_MOUSE_MOVE, (gui_cb)on_mouse_move);

	gui_window_show (ret->win);

	resize(ret, width_default, height_default);
	draw_parameter_fixed(ret, 0);
	draw_parameter_fixed(ret, 1);
	draw_parameter_fixed(ret, 2);

	return ret;
}

void asid_gui_view_free(asid_gui_view view) {
	if (view->gui->views == view)
		view->gui->views = view->next;
	else {
		asid_gui_view n = view->gui->views;
		while (n->next != view)
			n = n->next;
		n->next = view->next;
	}

	for (int i = 0; i < 4; i++) {
		free(view->screen_maps[i]);
		if (view->resized[i])
			free(view->resized[i]);
	}
	gui_window_free(view->win);
	free(view);
}

void asid_gui_view_resize_window(asid_gui_view view, uint32_t width, uint32_t height) {
	gui_window_resize(view->win, width, height);
}

void *asid_gui_view_get_handle(asid_gui_view view) {
	return gui_window_get_handle(view->win);
}

uint32_t asid_gui_view_get_width(asid_gui_view view) {
	return gui_window_get_width(view->win);
}

uint32_t asid_gui_view_get_height(asid_gui_view view) {
	return gui_window_get_height(view->win);
}

void asid_gui_view_on_timeout(asid_gui_view view) {
	draw(view);
}
