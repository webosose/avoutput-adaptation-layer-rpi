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

#include <iostream>
#include <cstring>
#include <vector>
#include <sstream>
#include <algorithm>
#include <inttypes.h>
#include <unistd.h>
#include <glib.h>
#include <fcntl.h>

#include <drm_fourcc.h>
#include <aval/aval_video.h>
#include "driElements.h"
#include "edid.h"

#define DRM_MODULE "vc4"

DRIElements::DRIElements(AVAL_VIDEO_SIZE_T defMode, std::function<void(AVAL_VIDEO_SIZE_T, AVAL_VIDEO_SIZE_T)> p)
			:mAvalCallBack(p)
			,mInitialMode(defMode)
			,mConfiguredMode(defMode)
{
	mUDev = new UDev([this](std::string node){updateDevice(node);});
	loadResources();

	auto devPair = mDeviceList.begin();
	if (devPair != mDeviceList.end())
	{
		DriDevice& device = devPair->second;
		device.setupDevice();

		updateDevice(devPair->first);

		for (auto& crtc : device.crtcList)
		{
			LOG_DEBUG("\n set Active mode for crtc %d %d", crtc.mCrtc->crtc_id, crtc.crtc_index);
			device.setActiveMode(crtc,device.width,device.height);
		}
		mPrimaryDev = devPair->first;
	}
	setupDevicePolling();
}

void DRIElements::loadResources()
{
	std::vector<std::string> uDevices = mUDev->getDeviceList();
	for (auto node : uDevices)
	{
		std::string udevNode = node;
		if (udevNode.find("card") == udevNode.npos)
		{
			continue;
		}

		//mDeviceList.emplace(udevNode, DriDevice{});
		mDeviceList.emplace(std::piecewise_construct, std::make_tuple(udevNode), std::make_tuple());
		DriDevice& device = mDeviceList[udevNode];
		device.deviceName = udevNode;
		device.drmModuleFd = open(udevNode.c_str(), O_RDWR | O_CLOEXEC);
		if (device.drmModuleFd < 0)
		{
			//THROW_FATAL_EXCEPTION("Failed to open the card %d", udevNode);
			LOG_ERROR(MSGID_DEVICE_ERROR,0, "Failed to open  %d", udevNode);
		}

		drmModeResPtr res = drmModeGetResources(device.drmModuleFd);
		if (!res)
		{
			LOG_ERROR(MSGID_DEVICE_ERROR,0,"Failed to get drm resources for %s", device.deviceName);
		}

		//build crtc list
		for (int i = 0; i < res->count_crtcs; i++)
		{
			DrmCrtc drmCrtc(drmModeGetCrtc(device.drmModuleFd, res->crtcs[i]), static_cast<uint32_t>(i));
			device.crtcList.push_back(drmCrtc);
		}
		//build connector list
		for (int i = 0; i < res->count_connectors; i++)
		{
			drmModeConnector* connector = drmModeGetConnector(device.drmModuleFd, res->connectors[i]);
			DrmConnector drmConnector(device.drmModuleFd, connector);
			device.connectorList.push_back(drmConnector);
		}

		//build encoder list
		for (int i = 0; i < res->count_encoders; i++)
		{
			DrmEncoder drmEncoder(drmModeGetEncoder(device.drmModuleFd, res->encoders[i]));
			device.encoderList.push_back(drmEncoder);
		}

		//build plane list
		drmModePlaneResPtr planeRes = drmModeGetPlaneResources(device.drmModuleFd);

		if (!planeRes)
		{
			LOG_ERROR(MSGID_DEVICE_ERROR,0,"drmModeGetPlaneResources failed: %s\n",
			          strerror(errno));
		}


		for (size_t i = 0; i < planeRes->count_planes; i++)
		{
			drmModePlane* plane = drmModeGetPlane(device.drmModuleFd, planeRes->planes[i]);
			DrmPlane drmPlane(plane);
			device.planeList.push_back(drmPlane);
		}

	}
}

void DRIElements::updateDevice(std::string name) //callback from udev
{

	LOG_DEBUG("Update device called \n************************\n");
	AVAL_VIDEO_SIZE_T maxSize, minSize;
	auto devPair = mDeviceList.find(name);
	if ( devPair != mDeviceList.end())
	{
		AVAL_VIDEO_SIZE_T confMode;
		DriDevice& device = devPair->second;
		if (mConfiguredMode.w != mInitialMode.w || mConfiguredMode.h != mInitialMode.h)
		{
			confMode.w = mInitialMode.w;
			confMode.h = mInitialMode.h;
		}
		else
		{
			confMode.w = mConfiguredMode.w;
			confMode.h = mConfiguredMode.h;
		}

		device.geModeRange(minSize, maxSize);
		if ((maxSize.w < confMode.w|| maxSize.h < confMode.h) &&
		    maxSize.w!=0 && maxSize.h !=0)
		{
			device.width = maxSize.w;
			device.height = maxSize.h;
		}
		else
		{
			device.width = confMode.w;
			device.height = confMode.h;
		}

		mAvalCallBack(minSize, maxSize);


	} else{
		LOG_ERROR(MSGID_DEVICE_ERROR, 0, "Cannot handle new DRM device detected %s", name.c_str());
	}
}

int DRIElements::changeMode(uint32_t width, uint32_t height, uint32_t vRefresh)
{
	//RPI has Single card, so use device
	DriDevice &device = mDeviceList[mPrimaryDev];
	//TODO:: When DSI1 crtc is also active/connected, changemode must be facilitate
	//changeMode api must change to either accept connector name or crtc id
	for (auto &crtc : device.crtcList)
	{
		//TODO::No need to iterate through crtc for now. remember crtc for hdmi
		if (!device.setActiveMode(crtc,width,height))
		{
			//TODO:: Once set this value is not used .. remove it?
			device.width = width;
			device.height = height;
			//change mConfigResolution instead
			mConfiguredMode.h = height;
			mConfiguredMode.w = width;
			return true;
		}
	}
	return false;
}

int DriDevice::geModeRange(AVAL_VIDEO_SIZE_T &minSize, AVAL_VIDEO_SIZE_T &maxSize)
{
	//Get the min and max from first connector to notify aval
	auto conn = connectorList.begin();
	if (conn->isPlugged())
	{
		LOG_DEBUG("isPlugged returned true ");
		DrmDisplayMode min, max;
		conn->getModeRange(min,max);
		maxSize.w = min.mModeInfoPtr->hdisplay;
		maxSize.h = min.mModeInfoPtr->vdisplay;

		minSize.w = max.mModeInfoPtr->hdisplay;
		minSize.h = max.mModeInfoPtr->vdisplay;
		LOG_DEBUG("\n max: %d x %d min: %d x %d", maxSize.w, maxSize.h, minSize.w,minSize.h);

		return 0;
	}
	return -1;
}

int DriDevice::setupDevice()
{
	hasDumbBuff();

	for (auto& conn : connectorList)
	{
		uint32_t crtcId = 0;
		uint32_t connId = conn.mConnectorPtr->connector_id;

		crtcId = findCrtc(conn);

		if (!crtcId)
		{
			LOG_ERROR(MSGID_DEVICE_ERROR, 0, "no valid crtc for connector %d",conn.mConnectorPtr->connector_id );
			continue;
		}

		//associate crtcId to connector. used if planes are updated based on connector name
		conn.setCrtcId(crtcId);
		//associate the connector to crtcz
		auto crtc = std::find_if (crtcList.begin(), crtcList.end(), [crtcId](DrmCrtc& c){ return c.mCrtc->crtc_id == crtcId; });
		if (crtc !=  crtcList.end())
		{
			crtc->connectors.insert(connId);
		}
	}
	return 0;
}

uint32_t DriDevice::findCrtc(DrmConnector &conn)
{
	drmModeEncoder *enc = nullptr;
	int32_t crtc = 0;
	/* try the currently conected encoder+crtc */
	if (conn.mConnectorPtr->encoder_id)
	{
		enc = drmModeGetEncoder(drmModuleFd, conn.mConnectorPtr->encoder_id);
	}
	if (enc)
	{
		if (enc->crtc_id)
		{
			crtc = enc->crtc_id;

		}
		drmModeFreeEncoder(enc);
		return crtc;
	}

	drmModeResPtr res = drmModeGetResources(drmModuleFd);

	/* if connector does not have encoder+crtc connected*/
	for (int i=0; i< conn.mConnectorPtr->count_encoders; i++)
	{
		enc =  drmModeGetEncoder(drmModuleFd,  conn.mConnectorPtr->encoders[i]);
		if (!enc)
		{
			LOG_DEBUG("encoder associated with connector not found");
			continue;
		}
		for (int j=0; j< res->count_crtcs; j++)
		{
			if (!(enc->possible_crtcs & (1 << j)))
			{
				continue;
			}
			crtc = res->crtcs[j];
			if (crtc >= 0) {
				break;
			}
		}
		drmModeFreeEncoder(enc);
	}
	return crtc;
}


int DriDevice::setActiveMode(DrmCrtc& crtc, const uint32_t width, const uint32_t height,const uint32_t vRefresh)
{
	std::stringstream modeStr;
	modeStr << width << "x" <<height;
	LOG_DEBUG("\n setActiveMode to %s", modeStr.str().c_str());
	//If there are no connectors dont set mode.
	if (!crtc.connectors.size())
	{
		LOG_INFO (MSGID_DEVICE_STATUS, 0, "No connectors set for crtc %d", crtc.mCrtc->crtc_id);
		return -1;
	}
	LOG_DEBUG("connectors has been set for crtc %d", crtc.mCrtc->crtc_id);

	DrmDisplayMode mode;
	//Check that all connectors connected to this crtc supports this mode.
	//Currently there is only 1 connector.
	for(auto connId: crtc.connectors)
	{
		auto conn = std::find_if(connectorList.begin(), connectorList.end(), [connId](DrmConnector &c)
		{ return c.mConnectorPtr->connector_id == connId; });
		if (!conn->isPlugged())
		{
			LOG_DEBUG("ignoring unused connector %d", connId);
			continue;
		}

		if (!conn->isModeSupported(modeStr.str()))
		{
			LOG_ERROR(MSGID_INVALID_DISPLAY_MODE, 0, "Mode %s is not supported by %d", modeStr.str(), conn->mConnectorPtr->connector_id);
			return -1;
		}

		if (!mode.mModeInfoPtr)
			mode = conn->getMode(modeStr.str());
	}

	if (!mode.mModeInfoPtr)
	{
		LOG_ERROR(MSGID_DISPLAY_NOT_CONNECTED, 0, "cannot get a valid mode object or connector not connected for crtc %d", crtc.mCrtc->crtc_id);
		return -1;
	}

	//create a new Fb if current fb size is different
	if (crtc.createScanoutFb(*this)) //create failed
	{
		return -1;
	}

	uint32_t* conn_ids = (uint32_t*) calloc(crtc.connectors.size(), sizeof(uint32_t));
	int index = 0;
	for (auto connId: crtc.connectors)
	{
		conn_ids[index++]=connId;
	}
	int ret = drmModeSetCrtc(drmModuleFd, crtc.mCrtc->crtc_id, crtc.scanout_fbId, 0, 0,
	                         conn_ids, 1, mode.mModeInfoPtr);
	if (ret)
	{
		LOG_ERROR(MSGID_DRM_MODESET_ERROR, 0, "Failed to set mode %d", ret);
	}
	return 0;
}

int DriDevice::hasDumbBuff()
{
	uint64_t has_dumb;

	if (drmGetCap(drmModuleFd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 )
	{
		THROW_FATAL_EXCEPTION("drm device  does not support dumb buffers!\n");
		return -EOPNOTSUPP;
	}
	if (!has_dumb)
	{
		THROW_FATAL_EXCEPTION("drm device  does not support dumb buffers!\n");
		return -EOPNOTSUPP;
	}
	return 0;
}

int DrmCrtc::createScanoutFb(DriDevice &device)
{

	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	unsigned int fb_id;

	int ret;
	if (boHandle && scanout_fbId != 0)
	{
		drmModeRmFB(device.drmModuleFd, scanout_fbId);
		bo_destroy(boHandle);
	}

	struct bo* bo = bo_create(device.drmModuleFd, DEFAULT_PIXEL_FORMAT, device.width,
	                          device.height, handles, pitches, offsets);
	if (!bo)
	{
		LOG_ERROR(MSGID_BUFFER_CREATION_FAILED, 0,"failed to create frame buffers  (%ux%u): (%d)", device.width, device.height, strerror(errno));
		return -errno;
	}

	//TODO:: set fourcc DRM_FORMAT_XRGB8888 as a config param
	ret = drmModeAddFB2(device.drmModuleFd, device.width, device.height,
	                    DRM_FORMAT_XRGB8888 , handles, pitches, offsets, &fb_id, 0);
	if (ret) {
		LOG_ERROR(MSGID_FB_CREATION_FAILED, 0, "failed to add fb (%ux%u): %s\n", device.width, device.height, strerror(errno));
		bo_destroy(bo);
		return ret;
	}

	scanout_fbId = fb_id;
	boHandle = bo;
	return 0;
}

DriDevice::~DriDevice()
{
	if (drmModuleFd)
	{
		//close(drmModuleFd);
	}
}

DRIElements::~DRIElements()
{
	g_source_remove(mTimeOutHandle);
	delete mUDev;
}

std::vector<uint32_t> DRIElements::getPlanes()
{
	DriDevice &driDevice = mDeviceList[mPrimaryDev];
	auto conn = driDevice.connectorList.begin();
	auto crtc = std::find_if(driDevice.crtcList.begin(), driDevice.crtcList.end(), [conn](DrmCrtc &c)
															{ return c.mCrtc->crtc_id == conn->crtc_id; });
	std::vector<uint32_t> planes;
	for (auto p : driDevice.planeList)
	{
		if (p.mDrmPlane->possible_crtcs  & (1 << crtc->crtc_index))
		{
			planes.push_back(p.mDrmPlane->plane_id);
		}
	}
	return planes;
}

bool DRIElements::setPlane(uint planeId, uint fbId, uint32_t crtc_x, uint32_t  crtc_y, uint32_t  crtc_w, uint32_t  crtc_h,
							  uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	LOG_DEBUG("Applying set plane to output {x:%u, y:%u, w:%u, h:%u} for source {x:%u, y:%u, w:%u, h:%u}, planeId %u",
	          crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w, src_h, planeId);

	DriDevice &driDevice = mDeviceList[mPrimaryDev];
	auto conn = driDevice.connectorList.begin();

	auto crtc = std::find_if(driDevice.crtcList.begin(), driDevice.crtcList.end(), [conn](DrmCrtc &c)
	{ return c.mCrtc->crtc_id == conn->crtc_id; });

	if (drmModeSetPlane(driDevice.drmModuleFd, planeId, crtc->mCrtc->crtc_id, fbId, 0,
                        crtc_x, crtc_y, crtc_w, crtc_h, src_x, src_y, src_w<<16, src_h<<16))
	{
		LOG_ERROR(MSGID_DRM_SET_PLANE_FAILED, 0, "%s", strerror(errno));
		return false;
	}

	return true;
}

std::vector<AVAL_VIDEO_SIZE_T> DRIElements::getSupportedModes()
{
	DriDevice &driDevice = mDeviceList[mPrimaryDev];
	auto conn = driDevice.connectorList.begin();
	//Get unique wxh values.
	if (conn !=  driDevice.connectorList.end())
	{
		auto modes = conn->getSupportedModes();

		modes.erase(std::unique(modes.begin(), modes.end(),
		            [] (const AVAL_VIDEO_SIZE_T& lhs, const AVAL_VIDEO_SIZE_T& rhs) {
			            return (lhs.w == rhs.w && lhs.h == rhs.h);}),modes.end());

		return modes;
	}
	return std::vector<AVAL_VIDEO_SIZE_T>();
}

bool DRIElements::setPlaneProperties( PLANE_PROPS_T propType, uint planeId, uint64_t value )
{

	DriDevice &driDevice = mDeviceList[mPrimaryDev];
	LOG_DEBUG("property type=%d, plane id = %d, value = %+" PRId64, propType, planeId, value);

	if (drmModeObjectSetProperty(driDevice.drmModuleFd, planeId, DRM_MODE_OBJECT_PLANE, propType, (uint64_t)value))
	{
		LOG_ERROR(MSGID_DRM_SET_PROP_FAILED, 0, "%s", strerror(errno));
		return false;
	}
	return true;

}

uint32_t DRIElements::getPlaneBase()
{
	return getPlanes()[0];
}

