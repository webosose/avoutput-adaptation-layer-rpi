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

#include <sstream>
#include "aval_impl.h"
#include "aval_audio_impl.h"
#include "aval_video_impl.h"
#include "aval_settings_impl.h"
#include "config.h"
#include "logging.h"

static const char* const logContextName = "aval-rpi";
static const char* const logPrefix = "[aval-rpi]";
PmLogContext avalLogContext;


AVAL* AVAL::_instance = nullptr;

AVAL* AVAL::getInstance()
{
	if(!_instance)
		_instance = new aval_impl();

	return _instance;
}


bool aval_impl::initialize()
{
	//Setup logging context
	PmLogErr error = PmLogGetContext(logContextName, &avalLogContext);
	if (error != kPmLogErr_None)
	{
		std::cerr << logPrefix << "Failed to setup up log context " << logContextName << std::endl;
	}

	audio = new aval_audio_impl(mDevCap);
	video = new aval_video_impl(mDevCap);
	controls = new AVAL_ControlSettings_Impl();

	return true;
}

bool aval_impl::deinitialize()
{
	delete audio;
	delete video;
	delete controls;
	return true;
}

std::string aval_impl::getConfigFilePath()
{
	std::stringstream configPath;
	configPath << CONFIG_DIR_PATH << "/" << "device-cap.json";
	LOG_DEBUG("AVAL Configuration file path: %s", configPath.str().c_str());
	return configPath.str();
}

