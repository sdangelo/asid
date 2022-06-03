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

#ifndef _VST3_PLUGIN_H
#define _VST3_PLUGIN_H

#include "config.h"

#include "public.sdk/source/vst/vstaudioeffect.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

class Plugin : AudioEffect {
public:
	Plugin();

	static FUnknown *createInstance(void *context) {
		return (IAudioProcessor *) new Plugin();
	}

	tresult PLUGIN_API initialize(FUnknown *context) SMTG_OVERRIDE;
	tresult PLUGIN_API terminate() SMTG_OVERRIDE;
	tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;
	tresult PLUGIN_API setupProcessing(ProcessSetup &setup) SMTG_OVERRIDE;
	tresult PLUGIN_API process(ProcessData &data) SMTG_OVERRIDE;
	tresult PLUGIN_API setBusArrangements(SpeakerArrangement *inputs, int32 numIns, SpeakerArrangement *outputs, int32 numOuts) SMTG_OVERRIDE;
	tresult PLUGIN_API setState(IBStream *state) SMTG_OVERRIDE;
	tresult PLUGIN_API getState(IBStream *state) SMTG_OVERRIDE;

private:
	float sampleRate;

	float parameters[NUM_PARAMETERS];
	int32 minBusesIn, minBusesOut; 

	P_TYPE instance;
	const float *inputs[NUM_CHANNELS_IN];
	float *outputs[NUM_CHANNELS_OUT];
};

#endif
