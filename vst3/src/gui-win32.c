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

extern "C" {

#include <stdio.h>
#include <stdlib.h>

#include "gui-win32.h"
#include "asid_gui.h"

static const char g_szClassName[] = "guiWindowClass";
static char keep_running;

static TRACKMOUSEEVENT tme = {0}; // Tmp
static char m_bMouseTracking = 0; // Tmp

static window firstWindow;

int instanceCounter = 0;

struct _window {
	window 			next;
	HWND 			handle;
	asid_gui 		gui;
	unsigned char* 	argb;
	HBITMAP 		map;
	uint32_t 		width, height;
};

static window findWin(HWND handle) {
	window winCur = firstWindow;
	while (winCur) {
		if (winCur->handle == handle)
			return winCur;
		winCur = winCur->next;
	}
	return NULL;
}

static void window_draw_ (HWND w, HBITMAP map, uint32_t srcX, uint32_t srcY, uint32_t destX, uint32_t destY, uint32_t width, uint32_t height) {
	// Temp HDC to copy picture
	HDC tmp = GetDC(NULL);
	HDC src = CreateCompatibleDC(tmp); // hdc - Device context for window
	SelectObject(src, map); // Inserting picture into our temp HDC
	HDC dest = GetDC(w);
	// Copy image from temp HDC to window
	BitBlt(
		dest, 			// Destination
		(int) destX,	// x and
		(int) destY,	// y - upper-left corner of place, where we'd like to copy
		(int) width,	// width of the region
		(int) height,
		src, 			// source
		(int) srcX, 	// x and
		(int) srcY, 	// y of upper left corner of part of the source, from where we'd like to copy
		SRCCOPY 		// Defined DWORD to juct copy pixels. Watch more on msdn;
	);
	ReleaseDC(w, dest);
	DeleteDC(src); 		// Deleting temp HDC
	DeleteDC(tmp);
}

static uint32_t get_mouse_state(WPARAM wParam) {
	return 
		(wParam & (MK_LBUTTON | MK_RBUTTON))
		| ((wParam & MK_MBUTTON) >> 2);
}


static LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	window w = findWin(hwnd);
	if (w == NULL)
		return 1;

	DWORD pos;
	uint32_t wt, ht;
	POINTS points;
    switch(msg)
    {
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        	points = MAKEPOINTS(lParam);
        	SetCapture(hwnd);
			asid_on_mouse_press(w->gui, points.x, points.y);
			//MessageBox(NULL, "Click", "Click!", MB_OK);
			break;
		case WM_LBUTTONUP:
    	case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        	points = MAKEPOINTS(lParam);
        	wt = get_mouse_state(wParam);
        	if (wt == 0)
        		ReleaseCapture();
			asid_on_mouse_release(w->gui);
			break;
		case WM_MOUSEMOVE:
			if (!m_bMouseTracking)
	        {
	            // Enable mouse tracking.
	            tme.cbSize = sizeof(tme);
	            tme.hwndTrack = hwnd;
	            tme.dwFlags = TME_HOVER | TME_LEAVE;
	            tme.dwHoverTime = HOVER_DEFAULT;
	            TrackMouseEvent(&tme);
	            m_bMouseTracking = 1;
	        }

        	points = MAKEPOINTS(lParam);
			asid_on_mouse_move(w->gui, points.x, points.y, get_mouse_state(wParam));
			break;
		case WM_MOUSELEAVE:
        	points = MAKEPOINTS(lParam);
		   	m_bMouseTracking = 0;
			break;
		case WM_KEYDOWN:
			break;
		case WM_KEYUP:
			break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
			break;
        case WM_DESTROY:
            PostQuitMessage(0);
			break;
		case WM_SIZE:
			wt = (uint32_t) LOWORD(lParam);
			ht = (uint32_t) HIWORD(lParam);
			if (wt > 0 && ht > 0) {
				//w->width = wt;
				//w->height = ht;
				//on_window_resize(ww, wt, ht);
			}
			break;
		case WM_PAINT:
			BeginPaint(hwnd, NULL);
			if (w != NULL && w->map != NULL) {
				window_draw_(hwnd, w->map, 0, 0, 0, 0, w->width, w->height);
			}
			EndPaint(hwnd, NULL);
			break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


int32_t gui_init () {
	instanceCounter++;
	
	WNDCLASSEX wc;

	wc.cbSize        = sizeof(wc);
    wc.style         = 0; // CS_HREDRAW | CS_VREDRAW; // redraw if size changes 
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = NULL;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    ATOM wcATOM = RegisterClassEx(&wc);
    if(!wcATOM) {
    	// Probably it got already registered
		return 1;
    }

    firstWindow = NULL;

	return 0;
}

/*
static uint32_t count_windows() {
	uint32_t counter = 0;
	if (firstWindow != NULL) {
		counter++;
		window winCur = firstWindow;
		while (winCur->next != NULL) {
			counter++;
			winCur = winCur->next;
		}
	}
	return counter;
}
*/

void gui_fini () {
	instanceCounter--;

	if (instanceCounter <= 0) { // Concurrency problem?
		instanceCounter = 0;
		UnregisterClass(g_szClassName, NULL);
	}
}


window gui_window_new (void* parent, uint32_t width, uint32_t height, asid_gui gui) {

	HWND* hwndParent = (HWND*) parent;

	HWND hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"ASID",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, NULL, NULL
	);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return NULL;
    }

	SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ! WS_BORDER & ! WS_SIZEBOX & ! WS_DLGFRAME );
	SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);


    tme.cbSize      = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags     = TME_LEAVE;
    tme.hwndTrack   = hwnd;
    TrackMouseEvent(&tme);

    UpdateWindow(hwnd);
  	SetParent(hwnd, *hwndParent);
    

	window newWindow = (window) malloc(sizeof(struct _window));
	if (newWindow == NULL)
		return NULL;
	newWindow->next = NULL;
	newWindow->handle = hwnd;
	newWindow->width = width;
	newWindow->height = height;
	newWindow->argb = NULL; /*(unsigned char*) malloc(4 * newWindow->width * newWindow->height);
	if (newWindow->argb == NULL) {
		MessageBox(NULL, "Memory error", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return NULL;
	}
	*/
	newWindow->map = NULL;
	newWindow->gui = gui;

	if (firstWindow == NULL)
		firstWindow = newWindow;
	else {
		window winCur = firstWindow;
		while (winCur->next != NULL)
			winCur = winCur->next;
		winCur->next = newWindow;
	}

    return newWindow;
   
}

void gui_window_free (window w) {
	if (w == firstWindow)
		firstWindow = w->next;
	else {
		window winCur = firstWindow;
		while (winCur->next != w)
			winCur = winCur->next;
		winCur->next = w->next;
	}

	//free(w->argb);
	DeleteObject(w->map);
	DestroyWindow(w->handle);
	free(w);
}


void gui_window_draw (window win, unsigned char *data, 
	uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, 
	uint32_t wx, uint32_t wy, uint32_t width, uint32_t height) 
{

	win->argb = data;

	DeleteObject(win->map);
	win->map = CreateBitmap(
		(int) win->width,
		(int) win->height,
		1, 				// color planes
		32, 			// Size of memory for one pixel in bits
		win->argb 		// src
	);

	BeginPaint(win->handle, NULL);
	window_draw_(win->handle, win->map, wx, wy, wx, wy, width, height);
	EndPaint(win->handle, NULL);
}


void *gui_window_get_handle(window w) {
	return w->handle;
}

void gui_window_move(window w, uint32_t x, uint32_t y) {
	SetWindowPos(w->handle, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void gui_window_resize(window w, uint32_t width, uint32_t height) {
	w->width = width;
	w->height = height;
	w->map = NULL;
	/*
	w->argb = (unsigned char*) realloc(w->argb, 4 * w->width * w->height);
	if (w->argb == NULL) {
		MessageBox(NULL, "Resize memory error", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	*/
	SetWindowPos(w->handle, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

uint32_t gui_window_get_width(window w) {
	return w->width;
}
uint32_t gui_window_get_height(window w) {
	return w->height;
}

void gui_window_show(window w) {
	ShowWindow(w->handle, SW_SHOW);
}

void gui_window_hide(window w) {
	ShowWindow(w->handle, SW_HIDE);
}

void gui_run () {
	keep_running = 1;
	MSG Msg;
	while(keep_running && GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
}

void gui_stop () {
	keep_running = 0;
}


void gui_set_timeout (window w, int32_t value) {
	SetTimer(
		w->handle,
		(UINT_PTR) NULL,
		(UINT) value,
		gui_on_timeout
	);
}


void gui_on_timeout (HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4) {
	window w = findWin(unnamedParam1);
	if (w == NULL)
		return;

	asid_on_timeout(w->gui);
}

}
