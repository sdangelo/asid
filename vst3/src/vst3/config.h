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

#ifndef _VST3_CONFIG_H
#define _VST3_CONFIG_H

// Definitions

#define IO_MONO			1
#define IO_STEREO		(1<<1)

struct config_io_bus {
	const char	*name;
	char		 out;
	char		 aux;
	char		 cv;
	char		 configs;
};

struct config_parameter {
	const char	*name;
	const char	*shortName;
	const char	*units;
	char		 out;
	char		 bypass;
	int		 steps;
	float		 defaultValueUnmapped;
};

// Data

#define COMPANY_NAME		"Orastron"
#define COMPANY_WEBSITE		"https://www.orastron.com/"
#define COMPANY_MAILTO		"mailto:info@orastron.com"

#define PLUGIN_NAME		"A-SID"
#define PLUGIN_VERSION		"1.0.1"
#define PLUGIN_SUBCATEGORY	"Fx|Filter"

#define PLUGIN_GUID_1		0x83AB6110
#define PLUGIN_GUID_2		0x11950DD3
#define PLUGIN_GUID_3		0x00944BFF
#define PLUGIN_GUID_4		0xF4F2ABBB

#define CTRL_GUID_1		0x9956378A
#define CTRL_GUID_2		0x7280FAA9
#define CTRL_GUID_3		0x286EFBB3
#define CTRL_GUID_4		0x99982AAA

#define NUM_BUSES_IN		1
#define NUM_BUSES_OUT		1
#define NUM_CHANNELS_IN		1
#define NUM_CHANNELS_OUT	1

static struct config_io_bus config_buses_in[NUM_BUSES_IN] = {
	{ "Audio in", 0, 0, 0, IO_MONO }
};

static struct config_io_bus config_buses_out[NUM_BUSES_OUT] = {
	{ "Audio out", 1, 0, 0, IO_MONO }
};

#define NUM_PARAMETERS	4

static struct config_parameter config_parameters[NUM_PARAMETERS] = {
	{ "Cutoff", "Cutoff", "", 0, 0, 0, 1.f },
	{ "LFO Amount", "LFO Amt", "%", 0, 0, 0, 0.f },
	{ "LFO Speed", "LFO Speed", "", 0, 0, 0, 0.5f },
	{ "Modulate Cutoff", "Mod Cutoff", "", 1, 0, 0, 0.f },
};

// Internal API

#include "asid.h"

#define P_TYPE				asid
#define P_NEW				asid_new
#define P_FREE				asid_free
#define P_SET_SAMPLE_RATE		asid_set_sample_rate
#define P_RESET				asid_reset
#define P_PROCESS			asid_process
#define P_SET_PARAMETER			asid_set_parameter
#define P_GET_PARAMETER			asid_get_parameter

#include "asid_gui.h"

#define PGUI_TYPE			asid_gui
#define PGUI_NEW			asid_gui_new
#define PGUI_FREE			asid_gui_free
#define PGUI_PROCESS_EVENTS		asid_gui_process_events
#define PGUI_GET_DEFAULT_WIDTH		asid_gui_get_default_width
#define PGUI_GET_DEFAULT_HEIGHT		asid_gui_get_default_height
#define PGUI_GET_DATA			asid_gui_get_data
#define PGUI_ON_PARAM_SET		asid_gui_on_param_set

#define PGUIVIEW_TYPE			asid_gui_view
#define PGUIVIEW_NEW			asid_gui_view_new
#define PGUIVIEW_FREE			asid_gui_view_free
#define PGUIVIEW_RESIZE_WINDOW		asid_gui_view_resize_window
#define PGUIVIEW_GET_HANDLE		asid_gui_view_get_handle
#define PGUIVIEW_GET_WIDTH		asid_gui_view_get_width
#define PGUIVIEW_GET_HEIGHT		asid_gui_view_get_height
#define PGUIVIEW_ON_TIMEOUT 		asid_gui_view_on_timeout

#endif
