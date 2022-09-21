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
 * File authors: Paolo Marrone
 */

#include "gui.h"

#import <Cocoa/Cocoa.h>

struct _window;

@interface GuiView : NSView {
    NSTrackingArea*         tracking;
    @public struct _window* win;
}
@end

struct _window {
  gui               g;
  struct _window*   next;
  NSWindow*         nswindow;
  GuiView*          view;
  unsigned char*    img;
  NSImage*          nsImage;
  uint16_t          width;
  uint16_t          height;
  void*             data;
  gui_cb            resize_cb;
  gui_cb            mouse_press_cb;
  gui_cb            mouse_release_cb;
  gui_cb            mouse_move_cb;
};

struct _gui {
  window windows;
  NSApplication* app;
  NSAutoreleasePool* pool;
};

@implementation GuiView

- (void) updateTrackingAreas {

  [self removeTrackingArea];
  NSTrackingAreaOptions options = 
      NSTrackingActiveAlways
    | NSTrackingInVisibleRect
    | NSTrackingMouseEnteredAndExited
    | NSTrackingMouseMoved;

  tracking = [[NSTrackingArea alloc] initWithRect:[self bounds]
    options:options
    owner:self
    userInfo:nil];

  [self addTrackingArea:tracking];
  [super updateTrackingAreas];
}

- (void) removeTrackingArea {
  if (tracking) {
    [super removeTrackingArea:tracking];
    [tracking release];
    tracking = 0;
  }
}

- (void) mouseDown : (NSEvent *) e {
  window w = self->win;
  NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
  if (w->mouse_press_cb)
    ((void(*)(window, int32_t, int32_t))w->mouse_press_cb)(w, (int32_t)p.x, self.frame.size.height - (int32_t)p.y);
}

- (void) mouseUp : (NSEvent *) e {
  window w = self->win;
  NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
  
  if (w->mouse_release_cb)
    ((void(*)(window, int32_t, int32_t))w->mouse_release_cb)(w, (int32_t)p.x, self.frame.size.height - (int32_t)p.y);
}

- (void) mouseMoved : (NSEvent *) e {
  window w = self->win;
  NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
  
  if (w->mouse_move_cb)
    ((void(*)(window, int32_t, int32_t, uint32_t))w->mouse_move_cb)(w, (int32_t)p.x, self.frame.size.height - (int32_t)p.y, 1);
}

- (void) mouseDragged : (NSEvent *) e {
  window w = self->win;
  NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
  
  if (w->mouse_move_cb)
    ((void(*)(window, int32_t, int32_t, uint32_t))w->mouse_move_cb)(w, (int32_t)p.x, self.frame.size.height - (int32_t)p.y, 1);
}

- (void) drawRect:(NSRect) r {
  [self->win->nsImage drawInRect:r fromRect:r operation:NSCompositingOperationCopy fraction:1.0];
}

@end

static void resized (window win, int32_t w, int32_t h) {
  if (win->resize_cb)
    ((void(*)(window, uint32_t, uint32_t))win->resize_cb)(win, w, h);
}

gui gui_new() {
  gui g = (gui) malloc(sizeof(struct _gui));
  g->windows = NULL;
  g->pool = [[NSAutoreleasePool alloc] init];
  g->app = [NSApplication sharedApplication];
  return g;
}

void gui_free(gui g) {
  free(g);
}

void gui_run(gui g, char single) {
  [g->app run];
}

void gui_stop (gui g) {
  [g->app stop:nullptr];
}

window gui_window_new(gui g, void* parent, uint32_t width, uint32_t height) { 

  window ret = (window) malloc(sizeof(_window));
  if (ret == NULL)
    return NULL;
  
  NSRect frame = NSMakeRect(0, 0, width, height);

  GuiView* view = [[GuiView alloc] initWithFrame:frame];
  view->win = ret;

  [((NSView*)parent) addSubview:view positioned:NSWindowAbove relativeTo:nil];

  [view autorelease];

  ret->g = g;
  ret->view = view;
  ret->nswindow = [(NSView*)parent window];
  ret->img = (unsigned char*) malloc(width * height * 4);
  ret->data = NULL;
  ret->nsImage = NULL;
  ret->width = width;
  ret->height = height;
  ret->next = NULL;

  if (g->windows == NULL)
    g->windows = ret;
  else {
    window cur = g->windows;
    while (cur->next != NULL)
      cur = cur->next;
    cur->next = ret;
  }

  return ret;
}

void gui_window_free(window w) {
  if (w->g->windows == w) {
    w->g->windows = w->next;
  }
  else {
    window cur = w->g->windows;
    while (cur != NULL && cur->next != w)
      cur = cur->next;
    if (cur == NULL)
      return;
    cur->next = w->next;
  }

  [w->view removeTrackingArea];
  free(w->img);
  free(w);
}

void gui_window_draw(window w, unsigned char* img, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh, uint32_t wx, uint32_t wy, uint32_t width, uint32_t height) {
  if (w->img == NULL)
    return;

  uint32_t iw = (wy * w->width + wx);
  uint32_t o =  (dy * dw       + dx);
  uint32_t p1 = (dw - width);
  uint32_t p2 = (w->width - width);

  const uint32_t* img32 = (uint32_t*) img;
  uint32_t* wimg32 = (uint32_t*) w->img;

  for (uint32_t y = dy; y < dy + height && y < dh; y++) {
    for (uint32_t x = dx; x < dx + width; x++, o++, iw++) {
      const uint32_t p = img32[o];
      wimg32[iw] = (p & 0xFF00FF00) | ((p >> 16) & 0x000000FF) | ((p << 16) & 0x00FF0000);
    }
    iw += p2;
    o  += p1;
  }

  CGDataProviderRef provider = CGDataProviderCreateWithData(
    NULL,               // void *info
    w->img,            // const void *data
    w->width * w->height * 4,  // size_t size
    NULL                // CGDataProviderReleaseDataCallback releaseData
  );

  CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
  CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
  CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;

  CGImageRef imageRef = CGImageCreate(
      w->width,         // size_t width
      w->height,        // size_t height 
      8,                // size_t bitsPerComponent
      4 * 8,            // size_t bitsPerPixel
      w->width * 4,     // size_t bytesPerRow
      colorSpaceRef,    // CGColorSpaceRef space
      bitmapInfo,       // CGBitmapInfo bitmapInfo
      provider,         // CGDataProviderRef provider
      NULL,             // const CGFloat *decode
      NO,               // bool shouldInterpolate
      renderingIntent   // CGColorRenderingIntent intent
  );

  NSImage* image = [[NSImage alloc] initWithCGImage: imageRef size:NSZeroSize];
  [w->nsImage release];
  w->nsImage = image;

  [w->view setNeedsDisplayInRect:NSMakeRect(wx, w->height - (wy + height), width, height)];

  CGDataProviderRelease(provider);
  CGColorSpaceRelease(colorSpaceRef);
  CGImageRelease(imageRef);
}

void gui_window_resize (window w, uint32_t width, uint32_t height) {
  NSSize newSize = NSMakeSize(width, height);
  [w->view setFrameSize:newSize];
  w->width = width;
  w->height = height;
  w->img = (unsigned char*)realloc(w->img, width * height * 4);

  resized(w, width, height);
}

void* gui_window_get_handle(window w) {
  return w->view;
}

uint32_t gui_window_get_width(window w) {
  return w->width;
}
uint32_t gui_window_get_height(window w) {
  return w->height;
}

void gui_window_show(window w) {
  // Can we? nswindow comes from outside
  //[w->nswindow setIsVisible:YES];
}

void gui_window_hide(window w) {
  //[w->nswindow setIsVisible:NO];
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
