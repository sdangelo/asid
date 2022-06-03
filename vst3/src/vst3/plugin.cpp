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


#include "plugin.h"

#include "pluginterfaces/base/conststringtable.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"

#include <algorithm>

#include <xmmintrin.h>
#include <pmmintrin.h>

Plugin::Plugin() {
	setControllerClass(FUID(CTRL_GUID_1, CTRL_GUID_2, CTRL_GUID_3, CTRL_GUID_4));
}

tresult PLUGIN_API Plugin::initialize(FUnknown *context) {
	instance = P_NEW();
	if (instance == nullptr)
		return kResultFalse;

	tresult r = AudioEffect::initialize(context);
	if (r != kResultTrue) {
		P_FREE(instance);
		return r;
	}

	// FIXME: vst3 sdk validator always seem to get kDefaultActive even in sdk plugins - it's probably broken, but let's check
	for (int i = 0; i < NUM_BUSES_IN; i++)
		addAudioInput(
			ConstStringTable::instance()->getString(config_buses_in[i].name),
			config_buses_in[i].configs & IO_STEREO ? SpeakerArr::kStereo : SpeakerArr::kMono,
			config_buses_in[i].aux ? kAux : kMain,
			(config_buses_in[i].cv ? BusInfo::kIsControlVoltage : 0)
			| (config_buses_in[i].aux ? 0 : BusInfo::kDefaultActive)
		);
	for (int i = 0; i < NUM_BUSES_OUT; i++)
		addAudioOutput(
			ConstStringTable::instance()->getString(config_buses_out[i].name),
			config_buses_out[i].configs & IO_STEREO ? SpeakerArr::kStereo : SpeakerArr::kMono,
			config_buses_out[i].aux ? kAux : kMain,
			(config_buses_out[i].cv ? BusInfo::kIsControlVoltage : 0)
			| (config_buses_out[i].aux ? 0 : BusInfo::kDefaultActive)
		);

	minBusesIn = 0;
	for (int i = 0; i < NUM_BUSES_IN; i++)
		if (!config_buses_in[i].aux)
			minBusesIn++;

	minBusesOut = 0;
	for (int i = 0; i < NUM_BUSES_OUT; i++)
		if (!config_buses_out[i].aux)
			minBusesOut++;

	for (int i = 0; i < NUM_PARAMETERS; i++) {
		parameters[i] = config_parameters[i].defaultValueUnmapped;
		P_SET_PARAMETER(instance, i, parameters[i]);
	}

	return kResultTrue;
}

tresult PLUGIN_API Plugin::terminate() {
	P_FREE(instance);

	return AudioEffect::terminate();
}

tresult PLUGIN_API Plugin::setActive(TBool state) {
	if (state) {
		P_SET_SAMPLE_RATE(instance, sampleRate);
		P_RESET(instance);
	}
	return AudioEffect::setActive(state);
}

tresult PLUGIN_API Plugin::setupProcessing(ProcessSetup &setup) {
	sampleRate = static_cast<float>(setup.sampleRate);
	return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API Plugin::process(ProcessData &data) {
	if (data.inputParameterChanges) {
		int32 n = data.inputParameterChanges->getParameterCount();
		for (int32 i = 0; i < n; i++) {
			IParamValueQueue *q = data.inputParameterChanges->getParameterData(i);
			if (!q)
				continue;
			ParamValue v;
			int32 o;
			if (q->getPoint(q->getPointCount() - 1, o, v) == kResultTrue) {
				int32 pi = q->getParameterId();
				parameters[pi] = v;
				P_SET_PARAMETER(instance, pi, std::min(std::max(static_cast<float>(v), 0.f), 1.f));
			}
		}
	}

	if (data.numInputs < minBusesIn || data.numOutputs < minBusesOut)
		return kResultOk;

	int k = 0;
	for (int i = 0; i < data.numInputs; i++)
		for (int j = 0; j < data.inputs[i].numChannels; j++, k++)
			inputs[k] = (const float *)data.inputs[i].channelBuffers32[j];
	for (; k < NUM_CHANNELS_IN; k++)
		inputs[k] = nullptr;
	
	k = 0;
	for (int i = 0; i < data.numOutputs; i++)
		for (int j = 0; j < data.outputs[i].numChannels; j++, k++)
			outputs[k] = data.outputs[i].channelBuffers32[j];
	for (; k < NUM_CHANNELS_OUT; k++)
		outputs[k] = nullptr;
	
	const int flush_zero_mode = _MM_GET_FLUSH_ZERO_MODE();
	const char denormals_zero_mode = _MM_GET_DENORMALS_ZERO_MODE();

	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

	P_PROCESS(instance, inputs, outputs, data.numSamples);
	
	_MM_SET_FLUSH_ZERO_MODE(flush_zero_mode);
	_MM_SET_DENORMALS_ZERO_MODE(denormals_zero_mode);

	// Send cutoff value to host
	IParameterChanges* outParamChanges = data.outputParameterChanges;
	if (outParamChanges) {
		float cutoffValue = asid_get_cutoff_modulated(instance);
		int32 index = 0;
		IParamValueQueue* paramQueue = outParamChanges->addParameterData (3, index);
		if (paramQueue)
		{
			int32 index2 = 0;
			paramQueue->addPoint (0, cutoffValue, index2);
		}
	}

	return kResultTrue;
}

tresult PLUGIN_API Plugin::setBusArrangements(SpeakerArrangement *inputs, int32 numIns, SpeakerArrangement *outputs, int32 numOuts) {
	if (numIns < minBusesIn || numIns > NUM_BUSES_IN)
		return kResultFalse;
	if (numIns < minBusesOut || numIns > NUM_BUSES_OUT)
		return kResultFalse;

	for (int32 i = 0; i < numIns; i++)
		if ((config_buses_in[i].configs == IO_MONO && inputs[i] != SpeakerArr::kMono)
		    || inputs[i] != SpeakerArr::kStereo)
			return kResultFalse;
	for (int32 i = 0; i < numOuts; i++)
		if ((config_buses_out[i].configs == IO_MONO && outputs[i] != SpeakerArr::kMono)
		    || outputs[i] != SpeakerArr::kStereo)
			return kResultFalse;

	return kResultTrue;
}

tresult PLUGIN_API Plugin::setState(IBStream *state) {
	if (!state)
		return kResultFalse;

	IBStreamer streamer(state, kLittleEndian);

	float f;
	for (int i = 0; i < NUM_PARAMETERS; i++) {
		if (streamer.readFloat(f) == false)
			return kResultFalse;
		parameters[i] = f;
		P_SET_PARAMETER(instance, i, f);
	}

	return kResultTrue;
}

tresult PLUGIN_API Plugin::getState(IBStream *state) {
	IBStreamer streamer(state, kLittleEndian);

	for (int i = 0; i < NUM_PARAMETERS; i++)
		streamer.writeFloat(parameters[i]);

	return kResultTrue;
}
