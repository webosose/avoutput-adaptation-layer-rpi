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

#include <cstdint>
#include <string>
#include <functional>
#include <aval_api.h>

class AVAL_ControlSettings_Impl:public AVAL_ControlSettings
{

public:
	AVAL_ControlSettings_Impl();
	bool configureVideoSettings(const std::string ctrl, AVAL_VIDEO_WID_T winID, const int32_t[]);
	bool configureSoundSettings(const std::string ctrl, const std::string );
	bool configureSoundSettings(const std::string control, const int32_t controlVal[]);
};