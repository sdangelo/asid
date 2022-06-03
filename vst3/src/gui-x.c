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

#ifdef __cplusplus
extern "C" {
#endif

#include "gui-x.h"


static xcb_connection_t *connection;
static xcb_screen_t *screen;
static const xcb_setup_t *setup;
static xcb_visualtype_t *visual;
static xcb_atom_t wm_protocols_atom;
static xcb_atom_t wm_delete_atom;
static xcb_atom_t xembed_info_atom;
static window windows;
static char keep_running;
static struct timespec ts_timeout;

int32_t gui_init() {
	connection = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(connection)) {
		xcb_disconnect(connection);
		return -1;
	}
	screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
	setup = xcb_get_setup(connection);

	xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
	xcb_depth_t *depth = NULL;
	while (depth_iter.rem) {
		if (depth_iter.data->depth == 24 && depth_iter.data->visuals_len) {
			depth = depth_iter.data;
			break;
		}
		xcb_depth_next(&depth_iter);
	}
	if (!depth){
		xcb_disconnect(connection);
		return -1;
	}

	xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth);
	visual = NULL;
	while (visual_iter.rem) {
		if (visual_iter.data->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
			visual = visual_iter.data;
			break;
		}
		xcb_visualtype_next(&visual_iter);
	}
	if (!visual) {
		xcb_disconnect(connection);
		return -1;
	}

	xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS"), NULL);
	if (!reply) {
		xcb_disconnect(connection);
		return -1;
	}
	wm_protocols_atom = reply->atom;
	free(reply);

	reply = xcb_intern_atom_reply(connection, xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW"), NULL);
	if (!reply) {
		xcb_disconnect(connection);
		return -1;
	}
	wm_delete_atom = reply->atom;
	free(reply);

	reply = xcb_intern_atom_reply(connection, xcb_intern_atom(connection, 0, 12, "_XEMBED_INFO"), NULL);
	if (!reply) {
		xcb_disconnect(connection);
		return -1;
	}
	xembed_info_atom = reply->atom;
	free(reply);

	windows = NULL;
	ts_timeout.tv_sec = -1;

	return 0;

err:
	// TBD: report
	xcb_disconnect(connection);
	return -1;
}

void gui_fini() {
	xcb_disconnect(connection);
}

#define XEMBED_MAPPED (1 << 0)

window gui_window_new(void* p, uint32_t width, uint32_t height, asid_gui gui) { // To fix
	xcb_window_t *parent = (xcb_window_t *) p;

	window ret = (window) malloc(sizeof(_window));
	if (ret == NULL)
		return NULL;
	
	ret->window = xcb_generate_id(connection);
	uint32_t mask = XCB_CW_EVENT_MASK;
	uint32_t values[] = { XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
			      | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
			      | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION
			      | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
			      | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE };
	xcb_create_window(connection, 24, ret->window, parent ? *parent : screen->root,
			  0, 0, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			  visual->visual_id, mask, values);
	
	xcb_change_property(connection, XCB_PROP_MODE_REPLACE, ret->window, wm_protocols_atom, XCB_ATOM_ATOM, 32, 1, &wm_delete_atom);

	ret->pixmap = xcb_generate_id(connection);
	xcb_create_pixmap(connection, screen->root_depth, ret->pixmap, ret->window, width, height);
	ret->gc = xcb_generate_id(connection);
	xcb_create_gc(connection, ret->gc, ret->pixmap, 0, NULL);

	uint32_t xembed_info[] = { 0, 0 };
	xcb_change_property(connection, XCB_PROP_MODE_REPLACE, ret->window, xembed_info_atom, xembed_info_atom, 32, 2, xembed_info);

	xcb_flush(connection);

	ret->next = NULL;
	if (windows == NULL)
		windows = ret;
	else {
		window n = windows;
		while (n->next)
			n = (window) n->next;
		n->next = ret;
	}

	ret->x = 0;
	ret->y = 0;
	ret->width = width;
	ret->height = height;
	
	return ret;
}

void gui_window_free(window w) {
	if (windows == w)
		windows = w->next;
	else {
		window n = windows;
		while ((window) n->next != w)
			n = (window) n->next;
		n->next = w->next;
	}

	xcb_free_gc(connection, w->gc);
	xcb_free_pixmap(connection, w->pixmap);
	xcb_destroy_window(connection, w->window);
	xcb_flush(connection);
	free(w);
}

void gui_window_draw(window w, unsigned char *data, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, uint32_t wx, uint32_t wy, uint32_t width, uint32_t height) {
	uint32_t bytes = 4 * width * height;
	unsigned char *argb = (unsigned char*) malloc(bytes);
	if (argb == NULL)
		return;

	uint32_t i = 0;
	uint32_t o = (dw * dy + dx) << 2;
	uint32_t p = (dw - width) << 2;
	if (setup->image_byte_order == XCB_IMAGE_ORDER_LSB_FIRST) {
		for (uint32_t y = dy; y < dy + height && y < dh; y++) {
			for (uint32_t x = dx; x < dx + width; x++, o += 4) {
				argb[i++] = data[o + 2];
				argb[i++] = data[o + 1];
				argb[i++] = data[o];
				i++;
			}
			o += p;
		}
	} else {
		for (uint32_t y = dy; y < dy + height; y++) {
			for (uint32_t x = dx; x < dx + width; x++, o += 4) {
				i++;
				argb[i++] = data[o];
				argb[i++] = data[o + 1];
				argb[i++] = data[o + 2];
			}
			o += p;
		}
	}

	xcb_put_image(connection, XCB_IMAGE_FORMAT_Z_PIXMAP, w->pixmap, w->gc, width, height, wx, wy, 0, screen->root_depth, bytes, argb);
	free(argb);
	xcb_copy_area(connection, w->pixmap, w->window, w->gc, wx, wy, wx, wy, width, height);
	xcb_flush(connection);
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
	xcb_configure_window(connection, w->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
	xcb_flush(connection);
}

void gui_window_show(window w) {
	uint32_t xembed_info[] = { 0, XEMBED_MAPPED };
	xcb_change_property(connection, XCB_PROP_MODE_REPLACE, w->window, xembed_info_atom, xembed_info_atom, 32, 2, xembed_info);
	xcb_map_window(connection, w->window);
	xcb_flush(connection);
}

void gui_window_hide(window w) {
	uint32_t xembed_info[] = { 0, 0 };
	xcb_change_property(connection, XCB_PROP_MODE_REPLACE, w->window, xembed_info_atom, xembed_info_atom, 32, 2, xembed_info);
	xcb_unmap_window(connection, w->window);
	xcb_flush(connection);
}

static window get_window(xcb_window_t xw) {
	window w = windows;
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

void gui_run(uint32_t single) {
	keep_running = 1;
	xcb_generic_event_t *ev;
	struct pollfd pfd;
	pfd.fd = xcb_get_file_descriptor(connection);
	pfd.events = POLLIN;
	struct timespec ts_now;
	int timeout;
	while (keep_running) {
		if (ts_timeout.tv_sec >= 0) {
			int err = clock_gettime(CLOCK_MONOTONIC, &ts_now);
			if (err == -1)
				break; // TODO: error
			if (ts_timeout.tv_sec < ts_now.tv_sec
			    || (ts_timeout.tv_sec == ts_now.tv_sec && ts_timeout.tv_nsec <= ts_now.tv_nsec)) {
				timeout = 0;
			}
			else
				timeout = 1000L * (ts_timeout.tv_sec - ts_now.tv_sec) + (ts_timeout.tv_nsec - ts_now.tv_nsec) / 1000000L;
		} else
			timeout = -1;
		if (timeout == 0) {
			ts_timeout.tv_sec = -1;
			//on_timeout();
			timeout = -1;
		}
		int err = poll(&pfd, 1, single ? 0 : timeout);
		if (err == -1)
			// TODO: error
			break;
		else if (err == 0) {
			if (single)
				break;
			else if (ts_timeout.tv_sec >= 0) {
				ts_timeout.tv_sec = -1;
				//on_timeout();
			}
		}

		while (keep_running && (ev = xcb_poll_for_event(connection))) {
			switch (ev->response_type & ~0x80) {
			case XCB_EXPOSE:
			{
				xcb_expose_event_t *x = (xcb_expose_event_t *)ev;
				window w = get_window(x->window);
	
				xcb_copy_area(connection, w->pixmap, w->window, w->gc, x->x, x->y, x->x, x->y, x->width, x->height);
				xcb_flush(connection);
			}
				break;
	
			case XCB_CONFIGURE_NOTIFY:
			{
				xcb_configure_notify_event_t *x = (xcb_configure_notify_event_t *)ev;
				window w = get_window(x->window);
	
				// FIXME: should translate to parent?
				xcb_translate_coordinates_reply_t *trans =
					xcb_translate_coordinates_reply(connection, xcb_translate_coordinates(connection, w->window, screen->root, 0, 0), NULL);
				if (trans) {
					if (trans->dst_x != w->x || trans->dst_y != w->y) {
						w->x = trans->dst_x;
						w->y = trans->dst_y;
						//on_window_move(w, w->x, w->y);
					}
					free(trans);
				}
	
				if (x->width != w->width || x->height != w->height) {
					w->width = x->width;
					w->height = x->height;
					xcb_free_pixmap(connection, w->pixmap);
					w->pixmap = xcb_generate_id(connection);
					xcb_create_pixmap(connection, screen->root_depth, w->pixmap, w->window, w->width, w->height);
					;//on_window_resize(w, w->width, w->height);
				}
			}
				break;
	
			case XCB_CLIENT_MESSAGE:
			{
				xcb_client_message_event_t *x = (xcb_client_message_event_t *)ev;
				window w = get_window(x->window);
	
				if (x->data.data32[0] == wm_delete_atom)
					;//on_window_close(w);
			}
				break;
	
			case XCB_BUTTON_PRESS:
			{
				xcb_button_press_event_t *x = (xcb_button_press_event_t *)ev;
				window w = get_window(x->event);
				// First three buttons
				if (x->detail <= 3) {
					//on_mouse_press(w, x->event_x, x->event_y, get_mouse_state(x->state, x->detail));
				}
				// Mouse wheel
				else {
					if (x->detail == 4)
						;//on_wheel(w, x->event_x, x->event_y, 57);
					else if (x->detail == 5)
						;//on_wheel(w, x->event_x, x->event_y, -57);
				}
			}
				break;
	
			case XCB_BUTTON_RELEASE:
			{
				xcb_button_release_event_t *x = (xcb_button_release_event_t *)ev;
				window w = get_window(x->event);

				// First three buttons
				if (x->detail <= 3) {
					//on_mouse_release(w, x->event_x, x->event_y, get_mouse_state(x->state, x->detail));
				}
				// Mouse wheel
				else {
					// Nothing to do
				}
			}
				break;
	
			case XCB_MOTION_NOTIFY:
			{
				xcb_motion_notify_event_t *x = (xcb_motion_notify_event_t *)ev;
				window w = get_window(x->event);
	
				//on_mouse_move(w, x->event_x, x->event_y, get_mouse_state(x->state, 0));
			}
				break;
	
			case XCB_ENTER_NOTIFY:
			{
				xcb_enter_notify_event_t *x = (xcb_enter_notify_event_t *)ev;
				window w = get_window(x->event);
	
				//on_mouse_enter(w, x->event_x, x->event_y, get_mouse_state(x->state, 0));
			}
				break;

			case XCB_LEAVE_NOTIFY:
			{
				xcb_leave_notify_event_t *x = (xcb_leave_notify_event_t *)ev;
				window w = get_window(x->event);
	
				//on_mouse_leave(w, x->event_x, x->event_y, get_mouse_state(x->state, 0));
			}
				break;
	
			case XCB_KEY_PRESS:
			{
				xcb_key_press_event_t *x = (xcb_key_press_event_t *)ev;
				window w = get_window(x->event);
	
				//on_key_press(w, x->detail, x->state);
			}
				break;
	
			case XCB_KEY_RELEASE:
			{
				xcb_key_release_event_t *x = (xcb_key_release_event_t *)ev;
				window w = get_window(x->event);
	
				//on_key_release(w, x->detail, x->state);
			}
				break;
	
			default:
				break;
			}
		
			free(ev);
		}
	}
}

void gui_stop() {
	keep_running = 0;
}

void gui_set_timeout(int32_t value) {
	int err = clock_gettime(CLOCK_MONOTONIC, &ts_timeout);
	if (err == -1)
		return; // TODO: error
	if (value < 0)
		ts_timeout.tv_sec = -1;
	else {
		long sec = value / 1000L;
		long nsec = 1000000L * (value - 1000L * sec);
		ts_timeout.tv_nsec += nsec;
		while (ts_timeout.tv_nsec >= 1000000000L) {
			ts_timeout.tv_sec++;
			ts_timeout.tv_nsec -= 1000000000L;
		}
		ts_timeout.tv_sec += sec;
	}
}

#ifdef __cplusplus
}
#endif
