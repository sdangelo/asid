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
 * File author: Stefano D'Angelo, Paolo Marrone
 */

#include "controller.h"

#include "pluginterfaces/base/conststringtable.h"
#include "base/source/fstreamer.h"

#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>

class PlugView : public EditorView {
public:
	PlugView(EditController *controller, PGUI_TYPE pgui) : EditorView(controller, nullptr) {
		this->pgui = pgui;
		pgui_view_created = 0;
	}

	~PlugView() {
	}

	tresult PLUGIN_API isPlatformTypeSupported(FIDString type) {
		return strcmp(type, kPlatformTypeHWND) ? kResultFalse : kResultTrue;
	}

	tresult PLUGIN_API attached(void *parent, FIDString type) {
		pgui_view = PGUIVIEW_NEW(pgui, &parent);
		pgui_view_created = 1;
		
		return kResultTrue;
	}

	tresult PLUGIN_API removed() {
		
		PGUIVIEW_FREE(pgui, pgui_view);
		pgui_view_created = 0;
		return kResultTrue;
	}

	tresult PLUGIN_API getSize(ViewRect *size) {
		if (!pgui_view_created) {
			*size = ViewRect(0, 0, PGUIVIEW_GET_DEFAULT_WIDTH(pgui), PGUIVIEW_GET_DEFAULT_HEIGHT(pgui));
			return kResultTrue;
		}
		*size = ViewRect(0, 0, PGUIVIEW_GET_WIDTH(pgui, pgui_view), PGUIVIEW_GET_HEIGHT(pgui, pgui_view));
		return kResultTrue;
	}

	tresult PLUGIN_API onSize(ViewRect *newSize) {
		if (!pgui_view_created)
			return kResultFalse;
		if (newSize)
			PGUIVIEW_RESIZE(pgui, pgui_view, newSize->getWidth(), newSize->getHeight());
		return kResultTrue;
	}

	tresult PLUGIN_API canResize() {
		return kResultTrue;
	}

private:
	PGUI_TYPE pgui;
	char pgui_view_created;
	PGUIVIEW_TYPE pgui_view;
};

/*
static void setParameterCb(void *handle, uint32_t id, float value) {
	EditController *c = (EditController *)handle;
	c->beginEdit(id);
	c->performEdit(id, value);
	c->endEdit(id);
}
*/
tresult PLUGIN_API Controller::initialize(FUnknown *context) {
	pgui = PGUI_NEW(this);
	if (pgui == nullptr)
		return kResultFalse;

	//PGUI_SET_SET_PARAMETER(pgui, setParameterCb);

	tresult r = EditController::initialize(context);

	// add parameters
	for (int i = 0; i < NUM_PARAMETERS; i++)
		parameters.addParameter(
			ConstStringTable::instance()->getString(config_parameters[i].name),
			config_parameters[i].units ? ConstStringTable::instance()->getString(config_parameters[i].units) : nullptr,
			config_parameters[i].steps,
			config_parameters[i].defaultValueUnmapped,
			(config_parameters[i].out ? ParameterInfo::kIsReadOnly | ParameterInfo::kIsHidden : ParameterInfo::kCanAutomate)
			| (config_parameters[i].bypass ? ParameterInfo::kIsBypass : 0),
			i,
			0,
			config_parameters[i].shortName ? ConstStringTable::instance()->getString(config_parameters[i].shortName) : nullptr
		);

	return kResultTrue;
}

tresult PLUGIN_API Controller::terminate() {
	PGUI_FREE(pgui);

	return EditController::terminate();
}

IPlugView * PLUGIN_API Controller::createView(const char *name) {
	if (strcmp(name, ViewType::kEditor))
		return nullptr;

	return new PlugView(this, pgui);
}

tresult PLUGIN_API Controller::setComponentState(IBStream *state) {
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	float f;
	for (int i = 0; i < NUM_PARAMETERS; i++) {
		if (streamer.readFloat(f) == false)
			return kResultFalse;
		setParamNormalized(i, f);
	}

	return kResultTrue;
}

tresult PLUGIN_API Controller::setParamNormalized(ParamID id, ParamValue value) {
	tresult r = EditController::setParamNormalized(id, value);
	if (r == kResultTrue)
		PGUI_ON_PARAM_SET(pgui, id, value);
	return r;
}

/*
tresult PLUGIN_API Controller::setState (IBStream* state) {
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::getState (IBStream* state) {
	return kResultTrue;
}
*/
