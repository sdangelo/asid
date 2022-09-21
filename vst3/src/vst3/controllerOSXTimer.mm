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

#include "controllerOSXTimer.h"

#import <Cocoa/Cocoa.h>

void* COSXSet_timer(int ms, void* cb, void* data) {
	NSTimer* t = [NSTimer timerWithTimeInterval:ms*0.001f repeats:YES block:^(NSTimer * _Nonnull timer) {
		((void(*) (void*)) cb)(data);
	}];
	[[NSRunLoop currentRunLoop] addTimer:t forMode:NSRunLoopCommonModes];
	return (void*) t;
}

void COSXRemove_timer(void* t) {
	if (t) {
		[(NSTimer*)t invalidate];
	}
}
