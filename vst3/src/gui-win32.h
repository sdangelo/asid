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

#ifndef _gui_WIN_H
#define _gui_WIN_H

#include <windows.h>
#include <stdint.h>


typedef struct _window* window;

#include "asid_gui.h"

int32_t gui_init ();
void gui_fini ();
window gui_window_new (void* wParent, uint32_t width, uint32_t height, asid_gui gui);
void gui_window_free (window w);
void gui_window_draw (window w, unsigned char *data, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, uint32_t wx, uint32_t wy, uint32_t width, uint32_t height);
void *gui_window_get_handle(window w);
void gui_window_move(window w, uint32_t x, uint32_t y);
void gui_window_resize(window w, uint32_t width, uint32_t height);
uint32_t gui_window_get_width(window w);
uint32_t gui_window_get_height(window w);
void gui_window_show(window w);
void gui_window_hide(window w);
void gui_run ();
void gui_stop ();
void gui_set_timeout (window w, int32_t value);
void gui_on_timeout (HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4);

#endif
