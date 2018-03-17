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


#include <string>
#include "drm_fourcc.h"
#include "driElements.h"

#include <iostream>
#include <algorithm>
#include <unistd.h>
#include "buffers.h"
#include "pattern.h"
#include "logging.h"
#include "aval/driElements.h"
#include <aval_api.h>


int main (int argc, const char *argv[])
{
	//Test program to initialize Drm structures and fill a pattern to scanout buffer.

	try
	{
		auto loop = g_main_loop_new (NULL, TRUE);

		AVAL_VIDEO_SIZE_T s; s.w=1920; s.h=1080;
		DRIElements driElements(s,
		                        [](AVAL_VIDEO_SIZE_T min, AVAL_VIDEO_SIZE_T max)
		                        {std::cout << "callback on device update";});

		if (driElements.mPrimaryDev != "")
		{
			DriDevice &driDevice = driElements.mDeviceList[driElements.mPrimaryDev];

			auto conn = driDevice.connectorList.begin();

			std::cout<<"CRTC for "<<conn->mConnectorPtr->connector_id << " =" << conn->crtc_id  << "**** \n";

			auto crtc = std::find_if(driDevice.crtcList.begin(), driDevice.crtcList.end(), [conn](DrmCrtc &c)
			{ return c.mCrtc->crtc_id == conn->crtc_id; });

			//struct bo *bo = driDevice.crtcList.at(2).boHandle;
			struct bo *bo = crtc->boHandle;

			//std::cout << "\n crtcid " << driDevice.crtcList.at(2).mCrtc->crtc_id;
			if (crtc != driDevice.crtcList.end())
			{
				std::cout << "\n crtcid " << crtc->mCrtc->crtc_id;
				std::cout << " " << driDevice.width << " " << driDevice.height << std::endl;
				std::cout << "bo & fbid" << bo << " " << driDevice.crtcList.at(2).scanout_fbId << std::endl;
			}

			fill_pattern(DEFAULT_PIXEL_FORMAT, bo, driDevice.width, driDevice.height, UTIL_PATTERN_TILES);

			for (auto p : driElements.getPlanes())
			{
				std::cout << "\n plane "<< p;
			}

		}
		std::cout << std::flush;
		g_main_loop_run(loop);

	}catch (FatalException e)
	{
		std::cout << "Fatal Exception" << e.what();
	}

}
