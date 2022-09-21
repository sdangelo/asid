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

#include <stdlib.h>
#include <windows.h>

#include "gui.h"

typedef struct _window {
	gui					 g;
	struct _window		*next;
	HWND 				 handle;
	HBITMAP 			 bitmap;
	BITMAPINFOHEADER	 bitmap_info;
	uint8_t				*bgra;
	char				 mouse_tracking;
	void				*data;
	gui_cb				 resize_cb;
	gui_cb				 mouse_press_cb;
	gui_cb				 mouse_release_cb;
	gui_cb				 mouse_move_cb;
} *window;

typedef struct _gui {
	char			 keep_running;
} *gui;

static const char* class_name = "guiWindowClass";
char gui_new_count = 0;
window windows = NULL;

static uint32_t get_mouse_state(WPARAM wParam) {
	return  (wParam & (MK_LBUTTON | MK_RBUTTON)) | ((wParam & MK_MBUTTON) >> 2);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	window w = windows;
	while (w && w->handle != hwnd)
		w = w->next;
	if (w == NULL)
		return DefWindowProc(hwnd, msg, wParam, lParam);

	DWORD pos;
	POINTS points;
    switch(msg)
    {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        	points = MAKEPOINTS(lParam);
        	SetCapture(hwnd);
			if (w->mouse_press_cb)
					((void(*)(window, int32_t, int32_t))w->mouse_press_cb)(w, points.x, points.y);
			break;
		case WM_LBUTTONUP:
    	case WM_MBUTTONUP:
        case WM_RBUTTONUP:
			points = MAKEPOINTS(lParam);
        	if (get_mouse_state(wParam) == 0)
        		ReleaseCapture();
			if (w->mouse_release_cb)
					((void(*)(window, int32_t, int32_t))w->mouse_release_cb)(w, points.x, points.y);
			break;
		case WM_MOUSEMOVE:
			if (!w->mouse_tracking)
	        {
				TRACKMOUSEEVENT tme;
	            tme.cbSize = sizeof(tme);
	            tme.hwndTrack = hwnd;
	            tme.dwFlags = TME_HOVER | TME_LEAVE;
	            tme.dwHoverTime = HOVER_DEFAULT;
	            TrackMouseEvent(&tme);
	            w->mouse_tracking = 1;
	        }
        	points = MAKEPOINTS(lParam);
			if (w->mouse_move_cb)
					((void(*)(window, int32_t, int32_t, uint32_t))w->mouse_move_cb)(w, points.x, points.y, get_mouse_state(wParam));
			break;
		case WM_MOUSELEAVE:
		   	w->mouse_tracking = 0;
			break;
		case WM_SIZE:
		{
			uint32_t width = LOWORD(lParam);
			uint32_t height = HIWORD(lParam);
			DeleteObject(w->bitmap);
			w->bitmap_info.biWidth = width;
			w->bitmap_info.biHeight = -height;
			HDC dc = GetDC(w->handle);
			w->bitmap = CreateDIBSection(dc, (BITMAPINFO*)&w->bitmap_info, DIB_RGB_COLORS, (void **)&w->bgra, NULL, 0);
			ReleaseDC(w->handle, dc);
			if (w->resize_cb)
					((void(*)(window, uint32_t, uint32_t))w->resize_cb)(w, width, height);
		}
			break;
		case WM_PAINT:
		{
			RECT r;
			if (GetUpdateRect(hwnd, &r, 0)) {
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(hwnd, &ps);
				SetDIBitsToDevice(dc, r.left, r.top, r.right - r.left, r.bottom - r.top, r.left, gui_window_get_height(w) - r.bottom, 0, -w->bitmap_info.biHeight, w->bgra, (BITMAPINFO*)&w->bitmap_info, DIB_RGB_COLORS);
				EndPaint(hwnd, &ps);
			}
		}
			break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

gui gui_new() {
	gui g = (gui)malloc(sizeof(struct _gui));
	if (!g)
		return NULL;
	
	if (!gui_new_count) {
		WNDCLASSEX wc;
		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style         = 0;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = NULL;
		wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(WHITE_BRUSH);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = class_name;
		wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
		RegisterClassEx(&wc);
	}
	gui_new_count++;

	return g;
}

void gui_free(gui g) {
	free(g);
	gui_new_count--;
	if (!gui_new_count)
		UnregisterClass(class_name, NULL);
}

void gui_run(gui g, char single) {
	g->keep_running = 1;
	MSG msg;
	while (g->keep_running) {
		if (single) {
			BOOL b = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
			if (!b)
				break;
		} else {
			BOOL b = GetMessage(&msg, NULL, 0, 0);
			if (b <= 0)
				break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void gui_stop(gui g) {
	g->keep_running = 0;
}

window gui_window_new(gui g, void* parent, uint32_t width, uint32_t height) {
	window w = (window)malloc(sizeof(struct _window));
	if (w == NULL)
		return NULL;

	w->handle = CreateWindowEx(0, class_name, NULL, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, NULL, NULL);
	if (w->handle == NULL) {
		free(w);
		return NULL;
	}
	
	ZeroMemory(&w->bitmap_info, sizeof(BITMAPINFOHEADER));
	w->bitmap_info.biSize = sizeof(BITMAPINFOHEADER);
	w->bitmap_info.biWidth = width;
	w->bitmap_info.biHeight = -height;
	w->bitmap_info.biPlanes = 1;
	w->bitmap_info.biBitCount = 32;
	w->bitmap_info.biCompression = BI_RGB;
	HDC dc = GetDC(w->handle);
	w->bitmap = CreateDIBSection(dc, (BITMAPINFO*)&w->bitmap_info, DIB_RGB_COLORS, (void **)&w->bgra, NULL, 0);
	ReleaseDC(w->handle, dc);
	if (w->bitmap == NULL) {
		DestroyWindow(w->handle);
		free(w);
		return NULL;
	}
	
	if (parent) {
		SetParent(w->handle, *((HWND *)parent));
		SetWindowLong(w->handle, GWL_STYLE, GetWindowLong(w->handle, GWL_STYLE) & ! WS_BORDER & ! WS_SIZEBOX & ! WS_DLGFRAME );
		SetWindowPos(w->handle, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	
	w->next = NULL;
	if (windows == NULL)
		windows = w;
	else {
		window n = windows;
		while (n->next)
			n = n->next;
		n->next = w;
	}
	
	TRACKMOUSEEVENT tme;
	tme.cbSize = sizeof(TRACKMOUSEEVENT);
	tme.dwFlags = TME_LEAVE;
	tme.hwndTrack = w->handle;
	TrackMouseEvent(&tme);
	w->mouse_tracking = 1;

	w->data = NULL;
	w->resize_cb = NULL;
	w->mouse_press_cb = NULL;
	w->mouse_release_cb = NULL;
	w->mouse_move_cb = NULL;
	
	UpdateWindow(w->handle);
	
	return w;
}

void gui_window_free(window w) {
	if (windows == w)
		windows = w->next;
	else {
		window n = windows;
		while (n->next != w)
			n = n->next;
		n->next = w->next;
	}
	
	DeleteObject(w->bitmap);
	DestroyWindow(w->handle);
	free(w);
}

void gui_window_draw(window w, unsigned char *data, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, uint32_t wx, uint32_t wy, uint32_t width, uint32_t height) {
	uint32_t *src = ((uint32_t *)data) + dw * dy + dx;
	uint32_t *dest = ((uint32_t *)w->bgra) + w->bitmap_info.biWidth * wy + wx;
	for (uint32_t h = height; h; h--, src += dw, dest += w->bitmap_info.biWidth)
		memcpy(dest, src, 4 * width);
	
	RECT r;
	r.left = wx;
	r.top = wy;
	r.right = wx + width;
	r.bottom = wy + height;
	RedrawWindow(w->handle, &r, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void gui_window_resize(window w, uint32_t width, uint32_t height) {
	SetWindowPos(w->handle, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void *gui_window_get_handle(window w) {
	return w->handle;
}

uint32_t gui_window_get_width(window w) {
	RECT r;
	GetWindowRect(w->handle, &r);
	return r.right - r.left;
}

uint32_t gui_window_get_height(window w) {
	RECT r;
	GetWindowRect(w->handle, &r);
	return r.bottom - r.top;
}

void gui_window_show(window w) {
	ShowWindow(w->handle, SW_SHOW);
}

void gui_window_hide(window w) {
	ShowWindow(w->handle, SW_HIDE);
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