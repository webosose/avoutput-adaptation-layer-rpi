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

#include <alsa/asoundlib.h>
#include <cmath>
#include "logging.h"
#include "aval_audio_impl.h"
#include "device_capability.h"


bool aval_audio_impl::initSpeaker()
{
	return true;
}

bool aval_audio_impl::connectInput(AVAL_AUDIO_RESOURCE_T audioResourceId, int16_t *port)
{
	LOG_DEBUG("connectInput  %d", audioResourceId);

	if (audioResourceId == AVAL_AUDIO_RESOURCE_MIXER0)
	{
		*port = 0;
	}
	else if (audioResourceId == AVAL_AUDIO_RESOURCE_MIXER1)
	{
		*port = 1;
	}
	return resetMixerVolume(audioResourceId, false);

}

bool aval_audio_impl::disconnectInput(AVAL_AUDIO_RESOURCE_T audioResourceId)
{
	return resetMixerVolume(audioResourceId, true);

}


bool aval_audio_impl::resetMixerVolume(AVAL_AUDIO_RESOURCE_T audioResourceId, bool mute)
{
	LOG_DEBUG("reset volume  resource=%d mute=%", audioResourceId, mute);

	//TODO::This is a temporary hack. Fix this when we implement different output types
	const char *card = mDeviceCapability.getAudioDefault().card.c_str();
	std::string controlId = "name=Softmaster";
	std::string volumeStr = "0%";
	if (!mute)
	{
		volumeStr = "100%";
	}

	if (audioResourceId == AVAL_AUDIO_RESOURCE_MIXER0)
	{
		controlId += "0";
	}
	else if (audioResourceId == AVAL_AUDIO_RESOURCE_MIXER1)
	{
		controlId += "1";
	}
	if (!setControl(card, controlId.c_str(), volumeStr.c_str()))
	{
		LOG_ERROR(MSGID_SET_VOLUME_ERROR, 0, "Failed to set control %s to %s for card:%s",controlId.c_str(), volumeStr.c_str(), card);
		return false;
	}
	return true;
}

bool aval_audio_impl::connectOutput(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_RESOURCE_T outputConnect, AVAL_AUDIO_RESOURCE_T currentConnect)
{
	return true;
}

bool aval_audio_impl::disconnectOutput(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_RESOURCE_T outputConnect, AVAL_AUDIO_RESOURCE_T currentConnect)
{
	return true;
}

bool aval_audio_impl::setMute(AVAL_AUDIO_RESOURCE_T audioResourceId, bool mute)
{
	return resetMixerVolume(audioResourceId, mute);
}


AVAL_ERROR aval_audio_impl::setOutputMode(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_SPDIF_MODE_T spdifMode)
{
	return AVAL_ERROR_NONE;
}

AVAL_ERROR aval_audio_impl::setOutputMute(AVAL_AUDIO_SNDOUT_T outputType, bool mute)
{
	LOG_DEBUG("In %s, setting mute to %d", __func__, mute);

	//TODO::This is a temporary hack. Fix this when we implement different output types
	const char *card = mDeviceCapability.getAudioDefault().card.c_str();
	std::string idstr = "name=";
	idstr+=mDeviceCapability.getAudioDefault().muteControlName.c_str();
	std::string mutestr = mute?"off":"on";
	if (!setControl(card, idstr.c_str(), mutestr.c_str()))
	{
		LOG_ERROR(MSGID_SET_VOLUME_ERROR,0, "Failed to set mute %s for card:%s, control:%s", mutestr, card, idstr.c_str());
		return AVAL_ERROR_FAIL;
	}


	return AVAL_ERROR_NONE;
}

AVAL_ERROR aval_audio_impl::setOutputVolume(AVAL_AUDIO_SNDOUT_T outputType, AVAL_AUDIO_VOLUME_T volume)
{
	//TODO: Add volume mapping curve - currently linear so no mapping needed
	if(!setVolume(volume))
	{
		LOG_ERROR(MSGID_SET_VOLUME_ERROR, 0, "Failed setting volume to %d", volume);
		return AVAL_ERROR_FAIL;
	}

	return AVAL_ERROR_NONE;
}

bool aval_audio_impl::setVolume(AVAL_AUDIO_VOLUME_T volume)
{
	const char* card = mDeviceCapability.getAudioDefault().card.c_str();
	std::string idstr = "name=";
	idstr += mDeviceCapability.getAudioDefault().volumeControlName.c_str();

	std::string volumeToSet = std::to_string(volume);
	volumeToSet += "%";

	LOG_DEBUG("In %s, setting volume to %s", __func__, volumeToSet.c_str());
	return setControl(card, idstr.c_str(), volumeToSet.c_str());
}

bool aval_audio_impl::setControl(const char* card, const char* idstr, const char* controlValue){
	int err;
	static snd_ctl_t *handle = NULL;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;

	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);

	if (snd_ctl_ascii_elem_id_parse(id, idstr))
	{
		LOG_DEBUG("Wrong control identifier: %s\n", idstr);
		return false;
	}

	LOG_DEBUG("snd_ctl_open on card %s, control %s value=%s\n", card, idstr, controlValue);
	if (handle == NULL &&
		(err = snd_ctl_open(&handle, card, 0)) < 0)
	{
		LOG_DEBUG("Control %s open error: %s\n", card, snd_strerror(err));
		return false;
	}

	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0)
	{
		LOG_DEBUG("Cannot find the given element from control %s, error %s\n", card, snd_strerror(err));
		snd_ctl_close(handle);
		handle = NULL;
		return false;
	}

	err = snd_ctl_ascii_value_parse(handle, control, info, controlValue);
	if (err < 0)
	{
		LOG_DEBUG("Control %s parse error: %s\n", card, snd_strerror(err));
		snd_ctl_close(handle);
		handle = NULL;
		return false;
	}

	if ((err = snd_ctl_elem_write(handle, control)) < 0) {
		LOG_DEBUG("Control %s element write error: %s\n", card, snd_strerror(err));
		snd_ctl_close(handle);
		handle = NULL;
		return false;
	}

	snd_ctl_close(handle);
	handle = NULL;
	return true;
}

