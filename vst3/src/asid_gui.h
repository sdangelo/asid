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
 * File authors: Stefano D'Angelo, Paolo Marrone
 */


#ifndef _ASID_GUI_H
#define _ASID_GUI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct _asid_gui* asid_gui;

#ifdef _WIN32

#include "gui-win32.h"

#endif

#ifdef linux

#include "gui-x.h"

#endif

asid_gui asid_gui_new(void *handle);
void asid_gui_free(asid_gui gui);
void asid_gui_set_set_parameter(asid_gui gui, void (*set_parameter)(void *handle, uint32_t id, float value));
void asid_gui_on_param_set(asid_gui gui, uint32_t id, float value);

typedef window asid_gui_view;

asid_gui_view asid_gui_view_new(asid_gui gui, void *parent);
void asid_gui_view_free(asid_gui gui, asid_gui_view view);
uint32_t asid_gui_view_get_width(asid_gui gui, asid_gui_view view);
uint32_t asid_gui_view_get_height(asid_gui gui, asid_gui_view view);
void asid_gui_view_resize(asid_gui gui, asid_gui_view view, uint32_t width, uint32_t height);
uint32_t asid_gui_view_get_default_width(asid_gui gui);
uint32_t asid_gui_view_get_default_height(asid_gui gui);

void asid_on_mouse_press (asid_gui gui, int x, int y);
void asid_on_mouse_release (asid_gui gui);
void asid_on_mouse_move (asid_gui gui, int x, int y, uint32_t mouseState);
void asid_on_timeout (asid_gui gui);

#ifdef __cplusplus
}
#endif

#endif
