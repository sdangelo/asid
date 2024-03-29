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

#include "gui.h"

#include <stdlib.h>
#include <xcb/xcb.h>
#include <poll.h>

typedef struct _window {
	gui		 g;
	_window		*next;
	xcb_window_t	 window;
	xcb_pixmap_t	 pixmap;
	xcb_gcontext_t	 gc;
	uint16_t	 width;
	uint16_t	 height;
	void		*data;
	gui_cb		 resize_cb;
	gui_cb		 mouse_press_cb;
	gui_cb		 mouse_release_cb;
	gui_cb		 mouse_move_cb;
} *window;

typedef struct _gui {
	xcb_connection_t	*connection;
	xcb_screen_t		*screen;
	const xcb_setup_t	*setup;
	xcb_visualtype_t	*visual;
	xcb_atom_t		 xembed_info_atom;
	window			 windows;
	char			 keep_running;
} *gui;

gui gui_new() {
	gui g = (gui) malloc(sizeof(struct _gui));
	if (g == NULL)
		return NULL;

	g->connection = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(g->connection)) {
		xcb_disconnect(g->connection);
		return NULL;
	}
	g->screen = xcb_setup_roots_iterator(xcb_get_setup(g->connection)).data;
	g->setup = xcb_get_setup(g->connection);

	xcb_visualtype_iterator_t visual_iter; xcb_intern_atom_reply_t* reply; // stupid C++ and its cross-initialization issues...

	xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(g->screen);
	xcb_depth_t *depth = NULL;
	while (depth_iter.rem) {
		if (depth_iter.data->depth == 24 && depth_iter.data->visuals_len) {
			depth = depth_iter.data;
			break;
		}
		xcb_depth_next(&depth_iter);
	}
	if (!depth)
		goto err;

	visual_iter = xcb_depth_visuals_iterator(depth);
	g->visual = NULL;
	while (visual_iter.rem) {
		if (visual_iter.data->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
			g->visual = visual_iter.data;
			break;
		}
		xcb_visualtype_next(&visual_iter);
	}
	if (!g->visual)
		goto err;

	reply = xcb_intern_atom_reply(g->connection, xcb_intern_atom(g->connection, 0, 12, "_XEMBED_INFO"), NULL);
	if (!reply)
		goto err;
	g->xembed_info_atom = reply->atom;
	free(reply);

	g->windows = NULL;

	return g;

err:
	gui_free(g);
	return NULL;
}

void gui_free(gui g) {
	xcb_disconnect(g->connection);
	free(g);
}

static window get_window(gui g, xcb_window_t xw) {
	window w = g->windows;
	while (w->window != xw)
		w = (window)w->next;
	return w;
}

static uint32_t get_mouse_state(uint16_t state, xcb_button_t last) {
	return (
		(state & XCB_BUTTON_MASK_1) >> 8
	| 	(state & XCB_BUTTON_MASK_2) >> 7 
	| 	(state & XCB_BUTTON_MASK_3) >> 9	)
	^	(last == 1 ? 1 : (last == 2 ? 4 : (last == 3 ? 2 : 0)))
	;
}

void gui_run(gui g, char single) {
	g->keep_running = 1;
	xcb_generic_event_t *ev;
	struct pollfd pfd;
	pfd.fd = xcb_get_file_descriptor(g->connection);
	pfd.events = POLLIN;
	while (g->keep_running) {
		int err = poll(&pfd, 1, single ? 0 : -1);
		if (err == -1)
			break;
		else if (err == 0 && single)
			break;

		while (g->keep_running && (ev = xcb_poll_for_event(g->connection))) {
			switch (ev->response_type & ~0x80) {
			case XCB_EXPOSE:
			{
				xcb_expose_event_t *x = (xcb_expose_event_t *)ev;
				window w = get_window(g, x->window);
	
				xcb_copy_area(g->connection, w->pixmap, w->window, w->gc, x->x, x->y, x->x, x->y, x->width, x->height);
				xcb_flush(g->connection);
			}
				break;
	
			case XCB_CONFIGURE_NOTIFY:
			{
				xcb_configure_notify_event_t *x = (xcb_configure_notify_event_t *)ev;
				window w = get_window(g, x->window);
	
				if (x->width != w->width || x->height != w->height) {
					w->width = x->width;
					w->height = x->height;
					xcb_free_pixmap(g->connection, w->pixmap);
					w->pixmap = xcb_generate_id(g->connection);
					xcb_create_pixmap(g->connection, g->screen->root_depth, w->pixmap, w->window, w->width, w->height);
					if (w->resize_cb)
						((void(*)(window, uint32_t, uint32_t))w->resize_cb)(w, w->width, w->height);
				}
			}
				break;
	
			case XCB_BUTTON_PRESS:
			{
				xcb_button_press_event_t *x = (xcb_button_press_event_t *)ev;
				window w = get_window(g, x->event);
				// First three buttons
				if (x->detail <= 3 && w->mouse_press_cb)
					((void(*)(window, int32_t, int32_t))w->mouse_press_cb)(w, x->event_x, x->event_y);
			}
				break;
	
			case XCB_BUTTON_RELEASE:
			{
				xcb_button_release_event_t *x = (xcb_button_release_event_t *)ev;
				window w = get_window(g, x->event);
				// First three buttons
				if (x->detail <= 3 && w->mouse_release_cb)
					((void(*)(window, int32_t, int32_t))w->mouse_release_cb)(w, x->event_x, x->event_y);
			}
				break;
	
			case XCB_MOTION_NOTIFY:
			{
				xcb_motion_notify_event_t *x = (xcb_motion_notify_event_t *)ev;
				window w = get_window(g, x->event);
				if (w->mouse_move_cb)
					((void(*)(window, int32_t, int32_t, uint32_t))w->mouse_move_cb)(w, x->event_x, x->event_y, get_mouse_state(x->state, 0));
			}
				break;
			}
		
			free(ev);
		}
	}
}

void gui_stop(gui g) {
	g->keep_running = 0;
}

#define XEMBED_MAPPED (1 << 0)

window gui_window_new(gui g, void* p, uint32_t width, uint32_t height) {
	xcb_window_t *parent = (xcb_window_t *) p;

	window ret = (window) malloc(sizeof(struct _window));
	if (ret == NULL)
		return NULL;
	
	ret->window = xcb_generate_id(g->connection);
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[] = { XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
			      | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
			      | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
			      | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
			      | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE };
	xcb_create_window(g->connection, 24, ret->window, parent ? *parent : g->screen->root,
			  0, 0, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  g->visual->visual_id, mask, values);
	
	ret->pixmap = xcb_generate_id(g->connection);
	xcb_create_pixmap(g->connection, g->screen->root_depth, ret->pixmap, ret->window, width, height);
	ret->gc = xcb_generate_id(g->connection);
	xcb_create_gc(g->connection, ret->gc, ret->pixmap, 0, NULL);

	uint32_t xembed_info[] = { 0, 0 };
	xcb_change_property(g->connection, XCB_PROP_MODE_REPLACE, ret->window, g->xembed_info_atom, g->xembed_info_atom, 32, 2, xembed_info);

	xcb_flush(g->connection);

	ret->next = NULL;
	if (g->windows == NULL)
		g->windows = ret;
	else {
		window n = g->windows;
		while (n->next)
			n = (window) n->next;
		n->next = ret;
	}

	ret->g = g;
	ret->width = width;
	ret->height = height;
	ret->data = NULL;
	ret->resize_cb = NULL;
	ret->mouse_press_cb = NULL;
	ret->mouse_release_cb = NULL;
	ret->mouse_move_cb = NULL;

	return ret;
}

void gui_window_free(window w) {
	if (w->g->windows == w)
		w->g->windows = w->next;
	else {
		window n = w->g->windows;
		while ((window) n->next != w)
			n = (window) n->next;
		n->next = w->next;
	}

	xcb_free_gc(w->g->connection, w->gc);
	xcb_free_pixmap(w->g->connection, w->pixmap);
	xcb_destroy_window(w->g->connection, w->window);
	xcb_flush(w->g->connection);
	free(w);
}

void gui_window_draw(window w, unsigned char *data, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, uint32_t wx, uint32_t wy, uint32_t width, uint32_t height) {
	uint32_t bytes = 4 * width * height;
	unsigned char *bgra = (unsigned char*) malloc(bytes);
	if (bgra == NULL)
		return;

	uint32_t i = 0;
	uint32_t o = (dw * dy + dx) << 2;
	uint32_t p = (dw - width) << 2;
	if (w->g->setup->image_byte_order == XCB_IMAGE_ORDER_MSB_FIRST) {
		for (uint32_t y = dy; y < dy + height && y < dh; y++) {
			for (uint32_t x = dx; x < dx + width; x++, o += 4) {
				bgra[i++] = data[o + 2];
				bgra[i++] = data[o + 1];
				bgra[i++] = data[o];
				i++;
			}
			o += p;
		}
	} else {
		for (uint32_t y = dy; y < dy + height; y++) {
			for (uint32_t x = dx; x < dx + width; x++, o += 4) {
				bgra[i++] = data[o];
				bgra[i++] = data[o + 1];
				bgra[i++] = data[o + 2];
				i++;
			}
			o += p;
		}
	}

	xcb_put_image(w->g->connection, XCB_IMAGE_FORMAT_Z_PIXMAP, w->pixmap, w->gc, width, height, wx, wy, 0, w->g->screen->root_depth, bytes, bgra);
	free(bgra);
	xcb_copy_area(w->g->connection, w->pixmap, w->window, w->gc, wx, wy, wx, wy, width, height);
	xcb_flush(w->g->connection);
}

void *gui_window_get_handle(window w) {
	return &w->window;
}

uint32_t gui_window_get_width(window w) {
	return w->width;
}
uint32_t gui_window_get_height(window w) {
	return w->height;
}

void gui_window_resize(window w, uint32_t width, uint32_t height) {
	const uint32_t values[] = { width, height };
	xcb_configure_window(w->g->connection, w->window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
	xcb_clear_area(w->g->connection, 1, w->window, 0, 0, width, height);
	xcb_flush(w->g->connection);
}

void gui_window_show(window w) {
	uint32_t xembed_info[] = { 0, XEMBED_MAPPED };
	xcb_change_property(w->g->connection, XCB_PROP_MODE_REPLACE, w->window, w->g->xembed_info_atom, w->g->xembed_info_atom, 32, 2, xembed_info);
	xcb_map_window(w->g->connection, w->window);
	xcb_flush(w->g->connection);
}

void gui_window_hide(window w) {
	uint32_t xembed_info[] = { 0, 0 };
	xcb_change_property(w->g->connection, XCB_PROP_MODE_REPLACE, w->window, w->g->xembed_info_atom, w->g->xembed_info_atom, 32, 2, xembed_info);
	xcb_unmap_window(w->g->connection, w->window);
	xcb_flush(w->g->connection);
}

void gui_window_set_data(window w, void *data) {
	w->data = data;
}

void *gui_window_get_data(window w) {
	return w->data;
}

void gui_window_set_cb(window w, gui_cb_type type, gui_cb cb) {
	switch (type) {
	case GUI_CB_RESIZE:
		w->resize_cb = cb;
		break;
	case GUI_CB_MOUSE_PRESS:
		w->mouse_press_cb = cb;
		break;
	case GUI_CB_MOUSE_RELEASE:
		w->mouse_release_cb = cb;
		break;
	case GUI_CB_MOUSE_MOVE:
		w->mouse_move_cb = cb;
		break;
	}
}
