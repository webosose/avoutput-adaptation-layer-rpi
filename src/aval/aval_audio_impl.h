// Copyright (c) 2017-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aval_api.h>
#include "device_capability.h"

class aval_audio_impl : public AVAL_Audio
{
private:
	bool initSpeaker();

public:

	aval_audio_impl(DeviceCapability &capability):mDeviceCapability(capability)
	{
		initSpeaker();
	}

	~aval_audio_impl() { }

	bool connectInput(AVAL_AUDIO_RESOURCE_T audioResourceId, int16_t*);
	bool disconnectInput(AVAL_AUDIO_RESOURCE_T audioResourceId);
	bool connectOutput(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_RESOURCE_T outputConnect, AVAL_AUDIO_RESOURCE_T currentConnect);
	bool disconnectOutput(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_RESOURCE_T outputConnect, AVAL_AUDIO_RESOURCE_T currentConnect);
	bool setMute(AVAL_AUDIO_RESOURCE_T audioResourceId, bool mute);
	AVAL_ERROR setOutputMode(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_SPDIF_MODE_T spdifMode);
	AVAL_ERROR setOutputMute(AVAL_AUDIO_SNDOUT_T outputType, bool mute);
	AVAL_ERROR setOutputVolume(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_VOLUME_T volume);

private:
	bool setVolume(AVAL_AUDIO_VOLUME_T volume);
	DeviceCapability &mDeviceCapability;
	bool setControl(const char* cardStr, const char* idStr, const char* volumeToSet);

	bool resetMixerVolume(AVAL_AUDIO_RESOURCE_T t, bool mute);
};