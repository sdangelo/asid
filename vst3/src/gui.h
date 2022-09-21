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

#ifndef _GUI_H
#define _GUI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _gui* gui;
typedef struct _window* window;
typedef void (*gui_cb)(void);

typedef enum {
	GUI_CB_RESIZE,
	GUI_CB_MOUSE_PRESS,
	GUI_CB_MOUSE_RELEASE,
	GUI_CB_MOUSE_MOVE
} gui_cb_type;

gui gui_new();
void gui_free(gui g);
void gui_run(gui g, char single);
void gui_stop(gui g);

window gui_window_new(gui g, void* parent, uint32_t width, uint32_t height);
void gui_window_free(window w);
void gui_window_draw(window w, unsigned char *bgra, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, uint32_t wx, uint32_t wy, uint32_t width, uint32_t height);
void gui_window_resize(window w, uint32_t width, uint32_t height);
void *gui_window_get_handle(window w);
uint32_t gui_window_get_width(window w);
uint32_t gui_window_get_height(window w);
void gui_window_show(window w);
void gui_window_hide(window w);
void gui_window_set_data(window w, void *data);
void *gui_window_get_data(window w);
void gui_window_set_cb(window w, gui_cb_type type, gui_cb cb);

#ifdef __cplusplus
}
#endif

#endif
