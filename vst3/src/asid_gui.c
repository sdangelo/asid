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
 * File author: Paolo Marrone
 */

#include "asid_gui.h"
#include <cstdio>

#include <sys/time.h>

#include "vst3/controller.h"

#ifdef __cplusplus
extern "C" {
#endif


#define PIXELED
#ifdef PIXELED
	#define RESIZE_FUNC pixeled_resize
#else
	#define RESIZE_FUNC bilinear_interpolation_resize
#endif

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


#include "screens.h"

static const uint8_t* screen_map_defaults[] = {screen1_map_default, screen2_map_default, screen3_map_default, screen4_map_default};

struct _asid_gui {
	void			*handle;
	
	asid_gui_view  	window; // Main window


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
	uint32_t f = floorI(x);
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


static void pixeled_resize (
	unsigned char *src, unsigned char *dest, 
	uint32_t src_w, uint32_t src_h,
	uint32_t src_target_x, uint32_t src_target_y, uint32_t src_target_w, uint32_t src_target_h,
	uint32_t dest_w, uint32_t dest_h,
	uint32_t dest_target_x, uint32_t dest_target_y, uint32_t dest_target_w, uint32_t dest_target_h)
{
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
}


static void bilinear_interpolation_resize (
	unsigned char *src, unsigned char *dest, 
	uint32_t src_w, uint32_t src_h,
	uint32_t src_target_x, uint32_t src_target_y, uint32_t src_target_w, uint32_t src_target_h,
	uint32_t dest_w, uint32_t dest_h,
	uint32_t dest_target_x, uint32_t dest_target_y, uint32_t dest_target_w, uint32_t dest_target_h)
{
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
}

static void draw_parameter_fixed(asid_gui gui, char p) {
	uint32_t pb_x1, pb_x2, yValue;
	uint32_t p_x1, p_x2;

	if (p == 0) {
		pb_x1 = parambox_cutoff_x1;
		pb_x2 = parambox_cutoff_x2;
		p_x1 = param_cutoff_x1;
		p_x2 = param_cutoff_x2;
		yValue = (uint32_t) ((1.f - gui->paramMappedValues[p]) * (float) params_inner_h) + params_inner_y;
	}
	else if (p == 1) {
		pb_x1 = parambox_lfoamt_x1;
		pb_x2 = parambox_lfoamt_x2;
		p_x1 = param_lfoamt_x1;
		p_x2 = param_lfoamt_x2;
		yValue = (uint32_t) ((1.f - gui->paramMappedValues[p]) * (float) params_inner_h) + params_inner_y;
	}
	else if (p == 2) {
		pb_x1 = parambox_lfospd_x1;
		pb_x2 = parambox_lfospd_x2;
		p_x1 = param_lfospd_x1;
		p_x2 = param_lfospd_x2;
		yValue = (uint32_t) ((1.f - gui->paramMappedValues[p]) * (float) params_inner_h) + params_inner_y;
	}
	else
		return;

	uint32_t index = ((paramboxs_y1) * inner_width_default + pb_x1) << 2;
	uint32_t wOffset = (inner_width_default - paramboxs_w) << 2;

	if (gui->param_hover == p) { // isHover
		for (uint32_t c = paramboxs_y1; c <= paramboxs_y2; c++) {
			for (int r = pb_x1; r <= pb_x2; r++) {
				if (screen_map_defaults[0][index] != 255) {
					for (char i = 0; i < 4; i++) {
						gui->screen_maps[i][index + 0] = colors_default[i][0];
						gui->screen_maps[i][index + 1] = colors_default[i][1];
						gui->screen_maps[i][index + 2] = colors_default[i][2];
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
						gui->screen_maps[i][index + 0] = screen_map_defaults[0][index + 0];
						gui->screen_maps[i][index + 1] = screen_map_defaults[0][index + 1];
						gui->screen_maps[i][index + 2] = screen_map_defaults[0][index + 2];
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
				gui->screen_maps[i][index + 0] = 255;
				gui->screen_maps[i][index + 1] = 255;
				gui->screen_maps[i][index + 2] = 255;
			}
			index += 4;
		}
		index += wOffset;
	}
}

static void draw_parameter_resized(asid_gui gui, char p) {
	uint32_t pb_x1, resized_x1;
	if (p == 0) {
		pb_x1 = parambox_cutoff_x1;
		resized_x1 = gui->xBoxCutoff;
	}
	else if (p == 1) {
		pb_x1 = parambox_lfoamt_x1;
		resized_x1 = gui->xBoxAmount;
	}
	else if (p == 2) {
		pb_x1 = parambox_lfospd_x1;
		resized_x1 = gui->xBoxSpeed;
	}
	else
		return;

	for (int ri = 0; ri < 4; ri++) {
		RESIZE_FUNC(
			gui->screen_maps[ri], gui->resized[ri], 
			inner_width_default, inner_height_default,  
			pb_x1, paramboxs_y1, paramboxs_w, paramboxs_h,
			gui->w, gui->h,
			resized_x1, gui->yBoxParams, gui->wBoxParams, gui->hBoxParams);
	}
}

static void update_gui_parameter(asid_gui gui, char p) {
	uint32_t resized_x1;
	if (p == 0) {
		resized_x1 = gui->xBoxCutoff;
	}
	else if (p == 1) {
		resized_x1 = gui->xBoxAmount;
	}
	else if (p == 2) {
		resized_x1 = gui->xBoxSpeed;
	}
	else
		return;

	gui_window_draw (gui->window, gui->resized[gui->screen_map_selected], 
		resized_x1, gui->yBoxParams, gui->w, gui->h, 
		resized_x1, gui->yBoxParams, gui->wBoxParams, gui->hBoxParams);
}

static void draw_modCutoff_slider_fixed (asid_gui gui) {
	static const uint32_t indexStart = (modCutoff_y1 * inner_width_default + modCutoff_x1) << 2;
	static const uint32_t wOffset = (inner_width_default - modCutoff_w) << 2; 
	const uint32_t yMid = modCutoff_y1 + (uint32_t) ((1.f - gui->modCutoffValue) * modCutoff_h);
	uint8_t* colors = color_grey;
	for (int ri = 0; ri < 4; ri++) {
		if (gui->param_selected == 0 || gui->param_hover == 0)
			colors = colors_default[ri];
		uint32_t index = indexStart;
		uint32_t r = modCutoff_y1;
		for (; r < yMid; r++) {
			for (uint32_t c = modCutoff_x1; c <= modCutoff_x2; c++) {
				gui->screen_maps[ri][index + 0] = 255; // Optimize: see array of char as array of int32.
				gui->screen_maps[ri][index + 1] = 255;
				gui->screen_maps[ri][index + 2] = 255;
				index += 4;
			}
			index += wOffset;
		}
		for (; r <= modCutoff_y2; r++) {
			for (uint32_t c = modCutoff_x1; c <= modCutoff_x2; c++) {
				gui->screen_maps[ri][index + 0] = colors[0];
				gui->screen_maps[ri][index + 1] = colors[1];
				gui->screen_maps[ri][index + 2] = colors[2];
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

void asid_gui_view_resize(asid_gui gui, asid_gui_view view, uint32_t width, uint32_t height) {
	float newrate = (float)width / (float) height;

	if (newrate >= imgrate) {
		gui->wWhite = (uint32_t) (imgrate * (float) height);
		gui->hWhite = height;
		gui->xWhite = (width - gui->wWhite) >> 1;
		gui->yWhite = 0;
	}
	else {
		gui->wWhite = width;
		gui->hWhite = (uint32_t) (((float) width) / imgrate);
		gui->xWhite = 0;
		gui->yWhite = (height - gui->hWhite) >> 1;
	}

	gui->scaleFactor = (float) gui->wWhite / (float) width_default;
	gui->scaleFactorInv = 1.f / gui->scaleFactor;
	uint32_t newPadding = (uint32_t) (((float) padding_default) * gui->scaleFactor);

	gui->xContent = gui->xWhite + newPadding;
	gui->yContent = gui->yWhite + newPadding;
	gui->wContent = gui->wWhite - (newPadding << 1);
	gui->hContent = gui->hWhite - (newPadding << 1);

	gui->xCutoff = (uint32_t) (gui->xContent + ((float) (param_cutoff_x1 + 1)) * gui->scaleFactor);
	gui->xAmount = (uint32_t) (gui->xContent + ((float) (param_lfoamt_x1 + 1)) * gui->scaleFactor);
	gui->xSpeed =  (uint32_t) (gui->xContent + ((float) (param_lfospd_x1 + 1)) * gui->scaleFactor);
	gui->yParams = (uint32_t) (gui->yContent + ((float) (params_y1 + 1)) * gui->scaleFactor);
	gui->wParams = (uint32_t) (((float) params_w) * gui->scaleFactor);
	gui->hParams = (uint32_t) (((float) params_h) * gui->scaleFactor);

	gui->xBoxCutoff = (uint32_t) (gui->xContent + ((float) (parambox_cutoff_x1)) * gui->scaleFactor); // Forse round è meglio
	gui->xBoxAmount = (uint32_t) (gui->xContent + ((float) (parambox_lfoamt_x1)) * gui->scaleFactor);
	gui->xBoxSpeed =  (uint32_t) (gui->xContent + ((float) (parambox_lfospd_x1)) * gui->scaleFactor);
	gui->yBoxParams = (uint32_t) (gui->yContent + ((float) (paramboxs_y1)) * gui->scaleFactor);
	gui->wBoxParams = ceilI (((float) paramboxs_w) * gui->scaleFactor);
	gui->hBoxParams = ceilI (((float) paramboxs_h) * gui->scaleFactor);

	gui->xModCutoff = gui->xContent + (uint32_t) (((float) modCutoff_x1) * gui->scaleFactor);
	gui->yModCutoff = gui->yContent + (uint32_t) (((float) modCutoff_y1) * gui->scaleFactor);
	gui->wModCutoff = ceilI (((float) modCutoff_w) * gui->scaleFactor);
	gui->hModCutoff = ceilI (((float) modCutoff_h) * gui->scaleFactor);

	gui->w = width;
	gui->h = height;

	gui->toResize = 1;
}

static void draw_resized (asid_gui gui) {
	const uint32_t size = (gui->w * gui->h) << 2;
	gui->resized[0] = (unsigned char*) realloc(gui->resized[0], size);
	gui->resized[1] = (unsigned char*) realloc(gui->resized[1], size);
	gui->resized[2] = (unsigned char*) realloc(gui->resized[2], size);
	gui->resized[3] = (unsigned char*) realloc(gui->resized[3], size);

	if (gui->resized[0] == NULL || gui->resized[1] == NULL || gui->resized[2] == NULL || gui->resized[3] == NULL) {
		return;
	}

	for (int ri = 0; ri < 4; ri++) {
		draw_padding(gui->resized[ri], 0, 0, gui->w, gui->h, gui->xWhite, gui->yWhite, gui->wWhite, gui->hWhite); 
		RESIZE_FUNC(
		  	gui->screen_maps[ri], gui->resized[ri], 
		  	inner_width_default, inner_height_default,  
		  	0, 0, inner_width_default, inner_height_default,
		  	gui->w, gui->h,
		  	gui->xContent, gui->yContent, gui->wContent, gui->hContent);
	}
}

static void update_gui_resize (asid_gui gui) {
	gui_window_resize(gui->window, gui->w, gui->h);
	gui_window_draw (gui->window, gui->resized[gui->screen_map_selected], 0, 0, gui->w, gui->h, 0, 0, gui->w, gui->h);
}

static void piggyback_draw(asid_gui gui) {
	if (gui == NULL || gui->window == NULL) {
		return;
	}

	for (int i = 0; i < 3; i++) {
		if (gui->paramToRedraw[i])
			draw_parameter_fixed(gui, i);
	}
	if (gui->modCutoffToRedraw)
		draw_modCutoff_slider_fixed(gui);

	if (gui->toResize) {
		draw_resized(gui);
		update_gui_resize(gui);
		for (int i = 0; i < 3; i++) 
			gui->paramToRedraw[i] = 1;
		gui->modCutoffToRedraw = 0;
		gui->toResize = 0;
	}
	else {
		for (int i = 0; i < 3; i++) {
			if (gui->paramToRedraw[i])
				draw_parameter_resized(gui, i);
		}
		if (gui->paramToRedraw[0] == 0 && gui->modCutoffToRedraw)
			draw_parameter_resized(gui, 0);

		if (gui->screen_map_selected != gui->screen_map_selected_old) {
			gui_window_draw (gui->window, gui->resized[gui->screen_map_selected], 
				gui->xContent, gui->yContent, gui->w, gui->h, 
				gui->xContent, gui->yContent, gui->wContent, gui->hContent);
			gui->screen_map_selected_old = gui->screen_map_selected;
		}
		else {
			for (int i = 0; i < 3; i++) {
				if (gui->paramToRedraw[i])
					update_gui_parameter(gui, i);
			}
			if (gui->paramToRedraw[0] == 0 && gui->modCutoffToRedraw)
				update_gui_parameter(gui, 0);
		}

		gui->paramToRedraw[0] = 0;
		gui->paramToRedraw[1] = 0;
		gui->paramToRedraw[2] = 0;
		gui->modCutoffToRedraw = 0;
	}
}

asid_gui asid_gui_new(void *handle) {
	asid_gui gui = (asid_gui)malloc(sizeof(struct _asid_gui));
	if (gui == NULL)
		return NULL;

	gui->handle = handle;
	gui->window = NULL; // Main and only window

	gui_init();

	gui->screen_maps[0] = (unsigned char*) malloc(inner_size_pixel);
	gui->screen_maps[1] = (unsigned char*) malloc(inner_size_pixel);
	gui->screen_maps[2] = (unsigned char*) malloc(inner_size_pixel);
	gui->screen_maps[3] = (unsigned char*) malloc(inner_size_pixel);

	memcpy(gui->screen_maps[0], screen_map_defaults[0], inner_size_pixel);
	memcpy(gui->screen_maps[1], screen_map_defaults[1], inner_size_pixel);
	memcpy(gui->screen_maps[2], screen_map_defaults[2], inner_size_pixel);
	memcpy(gui->screen_maps[3], screen_map_defaults[3], inner_size_pixel);

	gui->resized[0] = NULL;
	gui->resized[1] = NULL;
	gui->resized[2] = NULL;
	gui->resized[3] = NULL;

	gui->scaleFactor = 1.f;
	gui->scaleFactorInv = 1.f;

	gui->w = width_default;
	gui->h = height_default;

	gui->xWhite = 0;
	gui->yWhite = 0;
	gui->wWhite = width_default;
	gui->hWhite = height_default;

	gui->xContent = padding_default;
	gui->yContent = padding_default;
	gui->wContent = inner_width_default;
	gui->hContent = inner_height_default;

	gui->xCutoff = param_cutoff_x1;
	gui->xAmount = param_lfoamt_x1;
	gui->xSpeed  = param_lfospd_x1;
	gui->yParams = params_y1;
	gui->wParams = params_w;
	gui->hParams = params_h;

	gui->xBoxCutoff = parambox_cutoff_x1;
	gui->xBoxAmount = parambox_lfoamt_x1;
	gui->xBoxSpeed  = parambox_lfospd_x1;
	gui->yBoxParams = paramboxs_y1;
	gui->wBoxParams = paramboxs_w;
	gui->hBoxParams = paramboxs_h;

	gui->xModCutoff = modCutoff_x1;
	gui->yModCutoff = modCutoff_y1;
	gui->wModCutoff = modCutoff_w;
	gui->hModCutoff = modCutoff_h;

	gui->paramValues[0] = 1.f;
	gui->paramValues[1] = 0.f;
	gui->paramValues[2] = 0.5f;

	gui->paramMappedValues[0] = 1.f;
	gui->paramMappedValues[1] = 0.f;
	gui->paramMappedValues[2] = 0.46666666666666f;

	gui->param_hover = -1; // -1 = None, 0 = cutoff, 1 = amount, 2 = speed
	gui->param_selected = -1; // -1 = None, 0 = cutoff, 1 = amount, 2 = speed
	gui->mouse_old_y = 0;

	gui->screen_map_selected = 3;

	gui->modCutoffValue = 0.f;

	// Redrawing state
	gui->paramToRedraw[0] = 0;
	gui->paramToRedraw[1] = 0;
	gui->paramToRedraw[2] = 0;
	gui->modCutoffToRedraw = 0;
	gui->toResize = 0;

	return gui;
}

void asid_gui_free(asid_gui gui) {
	gui_fini();

	for (int i = 0; i < 4; i++) {
		free(gui->screen_maps[i]);
		free(gui->resized[i]);
	}
	free(gui);
}

asid_gui_view asid_gui_view_new(asid_gui gui, void *parent) {
	asid_gui_view view = gui_window_new (parent, width_default, height_default, gui);
	if (view == NULL)
		return NULL;
	gui->window = view;

	gui_window_show (view);

	asid_gui_view_resize(gui, view, 403, 283);
	draw_parameter_fixed(gui, 0);
	draw_parameter_fixed(gui, 1);
	draw_parameter_fixed(gui, 2);

	gui_set_timeout (view, 10);

	return view; 
}

void asid_gui_view_free(asid_gui gui, asid_gui_view view) {
	gui_window_free(view);
}

void asid_gui_on_param_set(asid_gui gui, uint32_t id, float value) {
	if (id == 3) {
		gui->modCutoffValue = value;
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

		gui->screen_map_selected = screen_map_selected_new;
		gui->modCutoffToRedraw = 1;
	}

	else {
		if (gui->paramToRedraw[id] == -1) // Set by the gui
			gui->paramToRedraw[id] = 1;
			return;
		gui->paramValues[id] = clipF(value, 0.f, 1.f);
		float newMappedV = floorI(gui->paramValues[id] * 15.f) * 0.0666666666666666f;
		if (gui->paramMappedValues[id] != newMappedV) {
			gui->paramMappedValues[id] = newMappedV;
			gui->paramToRedraw[id] = 1;
		}
	}
}

uint32_t asid_gui_view_get_width(asid_gui gui, asid_gui_view view) {
	return gui_window_get_width(view);
}

uint32_t asid_gui_view_get_height(asid_gui gui, asid_gui_view view) {
	return gui_window_get_height(view);
}

uint32_t asid_gui_view_get_default_width(asid_gui gui) {
	return width_default;
}

uint32_t asid_gui_view_get_default_height(asid_gui gui) {
	return height_default;
}


void asid_on_mouse_press (asid_gui gui, int x, int y) {
	x = (int) (((float) (x - (int) gui->xContent)) * gui->scaleFactorInv);
	y = (int) (((float) (y - (int) gui->yContent)) * gui->scaleFactorInv);

	if (y >= paramboxs_y1 && y <= paramboxs_y2) {
		if (x >= parambox_cutoff_x1 && x <= parambox_cutoff_x2) {
			gui->param_selected = 0;
		}
		else if (x >= parambox_lfoamt_x1 && x <= parambox_lfoamt_x2) {
			gui->param_selected = 1;
		}
		else if (x >= parambox_lfospd_x1 && x <= parambox_lfospd_x2) {
			gui->param_selected = 2;
		}
		else {
			gui->param_selected = -1;
		}
		gui->mouse_old_y = y;
	}
}

void asid_on_mouse_release (asid_gui gui) {
	gui->param_selected = -1;
	//gui->mouse_old_y = 0; // Maybe something better
}

void asid_on_mouse_move (asid_gui gui, int x, int y, uint32_t mouseState) {
	x = (int) (((float) (x - (int) gui->xContent)) * gui->scaleFactorInv);
	y = (int) (((float) (y - (int) gui->yContent)) * gui->scaleFactorInv);

	// Hover
	if (gui->param_selected == -1) {
		char old_param_hover = gui->param_hover;
		if (y >= paramboxs_y1 && y <= paramboxs_y2) {
			if (x >= parambox_cutoff_x1 && x <= parambox_cutoff_x2)
				gui->param_hover = 0;
			else if (x >= parambox_lfoamt_x1 && x <= parambox_lfoamt_x2)
				gui->param_hover = 1; 
			else if (x >= parambox_lfospd_x1 && x <= parambox_lfospd_x2)
				gui->param_hover = 2;
			else
				gui->param_hover = -1;
		}
		else
			gui->param_hover = -1;

		if (old_param_hover == gui->param_hover)
			;
		else {
			if (old_param_hover != -1)
				gui->paramToRedraw[old_param_hover] = 1;
			if (gui->param_hover != -1)
				gui->paramToRedraw[gui->param_hover] = 1;
			if (gui->paramToRedraw[0])
				gui->modCutoffToRedraw = 1;
		}
	}
	// Mouse diff
	if (gui->param_selected != -1 && mouseState > 0) {
		float diff = ((float) (gui->mouse_old_y - y)) / (float) params_h;

		if (diff != 0.f) {
			gui->paramValues[gui->param_selected] = clipF(gui->paramValues[gui->param_selected] + diff, 0.f, 1.f);
			float newMappedV = floorI(gui->paramValues[gui->param_selected] * 15.f) * 0.0666666666666666f;
			if (gui->paramMappedValues[gui->param_selected] != newMappedV) {
				gui->paramMappedValues[gui->param_selected] = newMappedV;
				gui->paramToRedraw[gui->param_selected] = -1;
				EditController *c = (EditController *) gui->handle;
				c->beginEdit(gui->param_selected);
				c->performEdit(gui->param_selected, gui->paramMappedValues[gui->param_selected]);
				c->endEdit(gui->param_selected);
			}
		}
	}
	gui->mouse_old_y = y;
}

void asid_on_timeout (asid_gui gui) {
	struct timeval stop, start;
	gettimeofday(&start, NULL);

	if (gui != NULL)
		piggyback_draw(gui);

	gettimeofday(&stop, NULL);

	const time_t elapsed = ((stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec) / 1000;
	uint32_t nextMs = elapsed < 20 ? 20 - elapsed : 1;

	gui_set_timeout (gui->window, nextMs);
}

#ifdef __cplusplus
}
#endif
