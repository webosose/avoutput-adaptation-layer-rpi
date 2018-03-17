// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "device_capability.h"
#include "logging.h"
#include <pbnjson/cxx/JDomParser.h>
#include "config.h"
using namespace pbnjson;

DeviceCapability::DeviceCapability(const std::string& configFilePath)
{
	// Load static DeviceCapability
	LOG_INFO(MSGID_LOAD_CONFIG, 0, "Loading static DeviceCapability from file %s", configFilePath.c_str());
	JValue configJson = JDomParser::fromFile(configFilePath.c_str(), JSchema::AllSchema());

	if (!configJson.isValid() || !configJson.isObject())
	{
		LOG_ERROR(MSGID_CONFIG_FILE_READ_FAILED, 0, "Failed to parse DeviceCapability file, using defaults. File: %s. Error: %s",
		          configFilePath.c_str(),
		          configJson.errorString().c_str());
	}
	else
	{
		// Parse the value, overriding defaults, so all values are optional.
		if (configJson.hasKey("videoCapabilities"))
		{
			JValue videoCapabilites = configJson["videoCapabilities"];
			if (!videoCapabilites.hasKey("maxResolution"))
			{
				LOG_ERROR(MSGID_CONFFILE_MISCONFIGURED, 0, "Failed to read mMaxResolution from DeviceCapability file, using default");
			}
			else
			{
				parseResolution(mMaxResolution, videoCapabilites["maxResolution"]);
			}

			if (!videoCapabilites.hasKey("minResolution"))
			{
				LOG_ERROR(MSGID_CONFFILE_MISCONFIGURED, 0,
				          "Failed to read minResolution from DeviceCapability file, using default.");
			}
			else
			{
				parseResolution(mMinResolution, videoCapabilites["minResolution"]);
			}

		}
		if (configJson.hasKey("planes"))
		{
			parsePlanes(configJson["planes"]);
		}
		if (configJson.hasKey("audioMasterDefault"))
		{
			LOG_DEBUG("Found audioMasterDefault");
			parseAudioDefaults(configJson["audioMasterDefault"]);
		}
		else {
			LOG_DEBUG("Did not find  audioMasterDefault");
		}

	}
}
void DeviceCapability::parseAudioDefaults(pbnjson::JValue object)
{
	if (!object.isObject())
	{
		LOG_ERROR(MSGID_CONFFILE_MISCONFIGURED, 0, "Failed to read resolution. using defaults.");
		return;
	}

	if (object.hasKey("card") && object.hasKey("muteControlName") && object.hasKey("volumeControlName"))
	{

		LOG_DEBUG("Found card muteControlName and volumeControlName");
		mAudioDefaults.card = object["card"].asString();
		mAudioDefaults.muteControlName = object["muteControlName"].asString();
		mAudioDefaults.volumeControlName = object["volumeControlName"].asString();
	}
}

void DeviceCapability::parseResolution(DeviceCapability::DeviceModeResolution &resolution, JValue object)
{
	if (!object.isObject())
	{
		LOG_ERROR(MSGID_CONFFILE_MISCONFIGURED, 0, "Failed to read resolution. using defaults.");
		return;
	}
	if (object.hasKey("w") && object.hasKey("h") && object.hasKey("freq"))
	{
		//What are max limits ever ? validate before assignment it here.
		resolution.w = static_cast<uint16_t>(object["w"].asNumber<int32_t>());
		resolution.h = static_cast<uint16_t>(object["h"].asNumber<int32_t>());
		resolution.freq = static_cast<uint16_t>(object["freq"].asNumber<int32_t>());
	}
}


void DeviceCapability::parsePlanes(pbnjson::JValue element)
{
	if (!element.isArray())
	{
		LOG_ERROR(MSGID_CONFFILE_MISCONFIGURED, 0, "Failed to read plane names. setting defaults.");
		return;
	}
	mPlaneNames.clear();
	for(auto pl:element.items())
	{
		mPlaneNames.insert(pl.asString());
	}
}

DeviceCapability::~DeviceCapability()
{
	LOG_DEBUG("Destroy DeviceCapability");
	//Do Nothing
}
