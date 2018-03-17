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

#include <algorithm>
#include <unordered_set>
#include <aval/aval_video.h>
#include <cinttypes>
#include "aval_video_impl.h"
#include "driElements.h"

AVAL_VIDEO_RECT_T aval_video_impl::getDisplayResolution()
{
	return AVAL_VIDEO_RECT_T{0, 0, 1920,1200};
}

aval_video_impl::aval_video_impl(DeviceCapability &deviceCapability)
				:mDeviceCapability(deviceCapability)
				,driElements(mDeviceCapability.getMaxResolution(),
				             [this](AVAL_VIDEO_SIZE_T min, AVAL_VIDEO_SIZE_T max)
				             {updatePlanes(min,max);})
{
	const std::set<std::string>& planeNames = mDeviceCapability.getPlaneNames();
	int wid = 0;

	for(auto& pstr : planeNames)
	{
		//TODO: window id should come from config file
		logicalPlanes.push_back(AVAL_PLANE_T{(AVAL_VIDEO_WID_T)wid++, pstr, mDeviceCapability.getMinResolution(), mDeviceCapability.getMaxResolution()});
	}

	//Acquire a pool of physical plane Ids (one per logical planes)
	std::vector<unsigned int> pplaneList = driElements.getPlanes();

	auto physicalPlaneId = pplaneList.begin();
	for(AVAL_PLANE_T &plane : logicalPlanes)
	{
		if(physicalPlaneId != pplaneList.end())
		{
			physicalPlanes.push_back(*physicalPlaneId);
			videoSinks.insert(std::make_pair(plane.wId, new SinkInfo(*physicalPlaneId)));
			physicalPlaneId++;
		}
	}

}

bool aval_video_impl::getVideoCapabilities(AVAL_VIDEO_SIZE_T& minDownscaleSize, AVAL_VIDEO_SIZE_T& maxUpscaleSize)
{
//TODO::vc4 crtc property has max and min resolution.(0x0-2048x2048)
// although tvservice/userland does not return this no matter what is connected

	maxUpscaleSize = mDeviceCapability.getMaxResolution();
	minDownscaleSize = mDeviceCapability.getMinResolution();
	return true;
}

std::vector<AVAL_PLANE_T> aval_video_impl::getVideoPlanes()
{
	return logicalPlanes;
}

bool aval_video_impl::isValidSink(AVAL_VIDEO_WID_T wId)
{
	if(videoSinks.find(wId) == videoSinks.end())
	{
		LOG_ERROR("INVALID_SINK", 0, "Invalid sink %d", wId);
		return false;
	}

	return true;
}

bool aval_video_impl::isSinkConnected(AVAL_VIDEO_WID_T wId)
{
	if(!isValidSink(wId))
	{
		return false;
	}

	return videoSinks[wId]->connected;
}

bool aval_video_impl::connect(AVAL_VIDEO_WID_T wId, AVAL_VSC_INPUT_SRC_INFO_T vscInput, AVAL_VSC_OUTPUT_MODE_T outputmode
					, unsigned int *planeId)
{
	if(!isValidSink(wId))
	{
		return false;
	}

	if(isSinkConnected(wId))
	{
		LOG_DEBUG("Sink %d already connected", wId);
		return true;
	}

	*planeId = videoSinks[wId]->planeId;
	videoSinks[wId]->connected=true;
	return true;
}

bool aval_video_impl::disconnect(AVAL_VIDEO_WID_T wId, AVAL_VSC_INPUT_SRC_INFO_T vscInput, AVAL_VSC_OUTPUT_MODE_T outputmode)
{
	LOG_DEBUG("disconnect called for wId %d", wId);
	if(!isSinkConnected(wId))
	{
		LOG_DEBUG("Sink %d is not connected", wId);
		return false;
	}
	videoSinks[wId]->connected=false;
	return true;
	if (!driElements.setPlaneProperties(SET_PLANE_FB_T, videoSinks[wId]->planeId,0))
	{
		LOG_ERROR(MSGID_VIDEO_DISCONNECT_FAILED, 0, "Faild to  set properties for wId %d", wId);
		return false;
	}
	return true;
}

typedef struct{
	/* Signed dest location allows it to be partially off screen */
	int32_t crtc_x, crtc_y;
	uint32_t crtc_w, crtc_h;

	/* Source values are 16.16 fixed point */
	uint32_t src_x, src_y;
	uint32_t src_h, src_w;
} scale_param_t;

scale_param_t scale_param;

bool aval_video_impl::applyScaling(AVAL_VIDEO_WID_T wId, AVAL_VIDEO_RECT_T srcInfo, bool adaptive, AVAL_VIDEO_RECT_T inputRegion, AVAL_VIDEO_RECT_T outputRegion)
{
	LOG_DEBUG("applyScaling called with srcInfo {x:%u, y:%u, w:%u, h:%u},"
              "inputRegion {x:%u, y:%u, w:%u, h:%u}, outputRegion {x:%u, y:%u, w:%u, h:%u}",
              srcInfo.x, srcInfo.y, srcInfo.w, srcInfo.h, inputRegion.x, inputRegion.y, inputRegion.w, inputRegion.h,
              outputRegion.x, outputRegion.y, outputRegion.w, outputRegion.h);

	if(!isSinkConnected(wId))
	{
		LOG_DEBUG("Sink %d is not connected", wId);
		return false;
	}

	scale_param = {
			outputRegion.x, outputRegion.y,
			outputRegion.w, outputRegion.h,
			inputRegion.x, inputRegion.y,
			inputRegion.h, inputRegion.w
	};

	LOG_DEBUG("Calling setPlaneProperties with scale_params %d, %d, %d, %d %d %d %d %d ", scale_param.crtc_x, scale_param.crtc_y, scale_param.crtc_w, scale_param.crtc_h,
					scale_param.src_x, scale_param.src_y, scale_param.src_w, scale_param.src_h);
	if (!driElements.setPlaneProperties(SET_SCALING_T, videoSinks[wId]->planeId, (uint64_t)&scale_param))
	{
		LOG_ERROR(MSGID_VIDEO_SCALING_FAILED, 0, "Failed to apply scaling for plane %d", videoSinks[wId]->planeId);
		return true;
	}


	return true;
}

bool aval_video_impl::setDualVideo(bool enable)
{//Do nothing.
	return true;
}

bool aval_video_impl::setCompositionParams(std::vector<AVAL_WINDOW_INFO_T> zOrder)
{

	for(size_t i=0; i<zOrder.size(); ++i)
	{
		LOG_DEBUG("zorder %d  for wId %d", i, zOrder[i].wId);
		if(!isValidSink(zOrder[i].wId))
		{
			return false;
		}
	}
	uint64_t zarg = 0;
	for(size_t i=0, p=physicalPlanes.size()-1; i<zOrder.size() && p>=0; ++i)
	{
		zarg = zarg <<16;
		zarg |= zOrder[i].wId;
	}

	uint16_t plane = driElements.getPlaneBase(); //plane must be any valid plane id.

	if (!driElements.setPlaneProperties(SET_Z_ORDER_T, plane,  zarg))
	{
		LOG_ERROR(MSGID_SET_ZORDER_FAILED, 0, "Failed to apply zorder for sink");
		return false;
	}
	return true;
}

bool aval_video_impl::setWindowBlanking(AVAL_VIDEO_WID_T wId, bool blank, AVAL_VIDEO_RECT_T inputRegion, AVAL_VIDEO_RECT_T outputRegion)
{
	if(!isSinkConnected(wId))
	{
		LOG_ERROR(MSGID_VIDEO_BLANKING_FAILED, 0, "Sink %d is not connected", wId);
		return false;
	}
	return true;

	if(blank)
	{
		/*FIX:PLAT-48894 Scaling is not working sometimes in Youtube app
		  We cannot initialize the values from the same object which is being constructed */
		scale_param_t scale_param {
				outputRegion.x, outputRegion.y,
				outputRegion.w, outputRegion.h,
				0,0,0,0
		};

		LOG_DEBUG("Calling setPlaneProperties with scale_params %d, %d, %d, %d %d %d %d %d ", scale_param.crtc_x, scale_param.crtc_y, scale_param.crtc_w, scale_param.crtc_h,
		          scale_param.src_x, scale_param.src_y, scale_param.src_h, scale_param.src_w);

		if (!driElements.setPlaneProperties(SET_SCALING_T, videoSinks[wId]->planeId, (uint64_t) &scale_param))
		{
			LOG_ERROR(MSGID_VIDEO_BLANKING_FAILED, 0, "Failed to blank wId %d", wId);
			return false;
		}
	}
	else
	{
		scale_param_t scale_param {
				outputRegion.x, outputRegion.y,
				outputRegion.w, outputRegion.h,
				inputRegion.x, inputRegion.y,
				inputRegion.h, inputRegion.w
		};
	
		LOG_DEBUG("Calling setPlaneProperties with scale_params %d, %d, %d, %d %d %d %d %d ", scale_param.crtc_x, scale_param.crtc_y, scale_param.crtc_w, scale_param.crtc_h,
		          scale_param.src_x, scale_param.src_y, scale_param.src_w, scale_param.src_h);

		if (!driElements.setPlaneProperties(SET_SCALING_T, videoSinks[wId]->planeId, (uint64_t) &scale_param))
		{
			LOG_ERROR(MSGID_VIDEO_UNBLANKING_FAILED, 0, "Failed to apply scaling for plane %d", videoSinks[wId]->planeId);
			return false;
		}
	}

	return true;
}

bool aval_video_impl::setDisplayResolution(AVAL_VIDEO_SIZE_T win)
{
	if (!isValidMode(win))
	{
		LOG_ERROR(MSGID_MODE_CHANGE_FAILED,0,"Invalid resolution specified %dx%d ",win.w,win.h);
		return false;
	}
	if (driElements.changeMode(win.w, win.h))
	{
		LOG_ERROR(MSGID_MODE_CHANGE_FAILED,0,"Resolution change failed %dx%d ",win.w,win.h);
		return false;
	}
	return true;
}

void aval_video_impl::updatePlanes(AVAL_VIDEO_SIZE_T min, AVAL_VIDEO_SIZE_T max) //callback function
{
	for (auto p : this->logicalPlanes)
	{
		if(mDeviceCapability.getMaxResolution().h >= max.h
		   || mDeviceCapability.getMaxResolution().w >= max.w)
		{
			p.maxSizeT = max;
		}if (mDeviceCapability.getMinResolution().h <= min.h
		     || mDeviceCapability.getMinResolution().w <= min.w)
		{
			p.minSizeT = min;
		}
	}
}

bool aval_video_impl::isValidMode(AVAL_VIDEO_SIZE_T win)
{
	//TODO::what other checks?
	if (win.w <= mDeviceCapability.getMaxResolution().w && win.h <= mDeviceCapability.getMaxResolution().h
		&& win.w >= mDeviceCapability.getMinResolution().w && win.h >= mDeviceCapability.getMinResolution().h)
	{
		return true;
	}
	return false;
}

std::vector<AVAL_VIDEO_SIZE_T> aval_video_impl::getSupportedResolutions()
{
	std::vector<AVAL_VIDEO_SIZE_T> modes = driElements.getSupportedModes();
	modes.erase(std::remove_if(modes.begin(), modes.end(), [this](AVAL_VIDEO_SIZE_T &m){return !isValidMode(m);}),
	            modes.end());
	return modes;

}
