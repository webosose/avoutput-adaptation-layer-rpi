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

#include <string>
#include <pbnjson/cxx/JValue.h>
#include <set>
#include <aval/aval_video.h>


class DeviceCapability
{
	class DeviceModeResolution
	{
	public:
		UINT16 w;
		UINT16 h;
		UINT16 freq; //Hz
		operator AVAL_VIDEO_SIZE_T(){
			AVAL_VIDEO_SIZE_T ret;ret.w=w; ret.h=h;
			return ret;

		}
	};

	class AudioDefaults
	{
	public:
		std::string card ="default";
		std::string muteControlName ="\'Master Playback Switch\'";
		std::string volumeControlName= "\'Master Playback Volume\'";
	};

public:
	DeviceCapability(const std::string &configFilePath);

	~DeviceCapability();

	DeviceCapability &operator=(const DeviceCapability &) = delete;

	DeviceCapability(const DeviceCapability &) = delete;

	AVAL_VIDEO_SIZE_T getMaxResolution() { return mMaxResolution;};
	AVAL_VIDEO_SIZE_T getMinResolution() { return mMinResolution;};
	AudioDefaults& getAudioDefault() {
		return mAudioDefaults;
	}
	const std::set<std::string>& getPlaneNames()
	{
		return mPlaneNames;
	};
private:

	AudioDefaults mAudioDefaults;

	DeviceModeResolution mMaxResolution ={
	w:1920,
	h:1080,
	freq:60};
	/*note: according to http://www.raspberrypi.org/phpBB3/viewtopic.php?f=26&t=20155&p=195417&hilit=2
   560x1600#p1954437 there is a pixel clock limit which means the highest supported mode is 1920x1200 @60 Hz with
	reduced blanking.*/
	DeviceModeResolution mMinResolution = {
			w:0,
			h:0,
			freq:60
	};
	/*note:in hdmi_safe mode w and h printed by tvservice is 640x480 which is listed the minimum res in device-cap.json*/

	std::set<std::string> mPlaneNames = {"MAIN"};
	void parseResolution(DeviceModeResolution &resolution, pbnjson::JValue object);
	void parsePlanes(pbnjson::JValue element);

	void parseAudioDefaults(pbnjson::JValue element);
};


/*according to http://www.raspberrypi.org/phpBB3/viewtopic.php?f=26&t=20155&p=195417&hilit=2560x1600#p195443
        there is a pixel clock limit which means the highest supported mode is 1920x1200 @60 Hz with reduced blanking.*/
/*obtained by setting hdmi_safe*/
//TODO:: is 240p the minimum
