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

#include <xf86drm.h>
#include <xf86drmMode.h>
#include "drm.h"
#include <sys/mman.h>
#include <iostream>
#include <string>
#include <vector>
#include <aval/aval_common.h>
#include <unordered_map>
#include <glib.h>
#include <set>
#include <aval/aval_video.h>
#include <functional>
#include "buffers.h"
#include "edid.h"
#include "logging.h"

#define DEFAULT_PIXEL_FORMAT DRM_FORMAT_XRGB8888
class DRIElements;
class DriDevice;
class DrmDisplayMode{
//TODO:: Remove this class and utils surrounding it
public:
	drmModeModeInfoPtr mModeInfoPtr = nullptr;
	operator drmModeModeInfoPtr()
	{
		return mModeInfoPtr;
	}

	DrmDisplayMode(drmModeModeInfoPtr modeInfo):mModeInfoPtr(modeInfo){};
	DrmDisplayMode(){};
	DrmDisplayMode(const DrmDisplayMode& dm)
	{
		mModeInfoPtr = dm.mModeInfoPtr;
	};
	DrmDisplayMode& operator= (const DrmDisplayMode& dm)
	{
		mModeInfoPtr = dm.mModeInfoPtr;
		return *this;
	};
	friend std::ostream& operator<< (std::ostream &os, const DrmDisplayMode &dm);
	friend DRIElements;
	friend DriDevice;
};

struct compare_AVAL_VIDEO_SIZE {
	bool operator() (const AVAL_VIDEO_SIZE_T& lhs, const AVAL_VIDEO_SIZE_T& rhs) const {
		return (lhs.w < rhs.w && lhs.h < rhs.h);
	}
};
struct DrmConnector
{

	DrmConnector(int fd, drmModeConnectorPtr pConnector);
	void copy(const DrmConnector &other)
	{ mConnectorPtr = other.mConnectorPtr; mProps = other.mProps;
		props_info = other.props_info; mDrmModulefd = other.mDrmModulefd;
		mName = other.mName;
		crtc_id = other.crtc_id;

	};
	DrmConnector(const DrmConnector &other)
	{
		copy(other);
	}
	DrmConnector& operator=(const DrmConnector &other)
	{
		copy(other); return *this;
	}

	DrmConnector(){};
	~DrmConnector(){};

	void setCrtcId(int id) {crtc_id = id;}

	bool isModeSupported(std::string mode_name, const uint32_t vRefresh=0);
	std::vector<AVAL_VIDEO_SIZE_T>  getSupportedModes();
	bool getModeRange(DrmDisplayMode& min, DrmDisplayMode& max);
	DrmDisplayMode getMode(const std::string mode_name, const uint32_t vRefresh=0);
	Edid getEdid();
	std::string getName(){
		return mName;
	}

	bool isPlugged();
	//void readProperties();

	int mDrmModulefd = -1; //is this needed
	uint32_t crtc_id = 0; //connected to crtc
	std::string mName;
	drmModeConnector *mConnectorPtr = nullptr;
	drmModeObjectProperties *mProps = nullptr;
	drmModePropertyRes **props_info;

	friend DRIElements;
	friend DriDevice;
};

class DrmEncoder
{
	drmModeEncoder *mEncoder = nullptr;
public:
	DrmEncoder(drmModeEncoder *encoder):mEncoder(encoder){};
};

struct DrmCrtc
{

	DrmCrtc(drmModeCrtc *crtc, uint32_t index):mCrtc(crtc),crtc_index(index){};

	void copy(const DrmCrtc &other)
	{
		mCrtc = other.mCrtc;
		boHandle = other.boHandle;
		scanout_fbId = other.scanout_fbId;
		connectors = other.connectors;
		crtc_index = other.crtc_index;
	}

	DrmCrtc(const DrmCrtc &crtc)
	{ copy(crtc);
	};

	DrmCrtc& operator=(const DrmCrtc &crtc)
	{ copy(crtc); return *this;};

	int createScanoutFb(DriDevice &device);

	drmModeCrtc *mCrtc = nullptr;
	std::set<uint32_t> connectors;
	uint32_t scanout_fbId=0;
	uint32_t crtc_index =0;
	struct bo *boHandle = nullptr;

	friend DRIElements;
};

struct DrmPlane {
	drmModePlane *mDrmPlane;

	DrmPlane(drmModePlane *drmPlane) : mDrmPlane(drmPlane) {}

	friend std::ostream& operator<< (std::ostream &os, const DrmPlane &dm);

};


void dumpProperties ( std::ostream &os, drmModePropertyPtr prop,
                      uint32_t prop_id, uint64_t value);

class DriDevice
{
public :
	std::string deviceName; //"/dev/dri/card0"
	int drmModuleFd = -1;

	std::vector<DrmConnector> connectorList;
	std::vector<DrmEncoder> encoderList;
	std::vector<DrmCrtc> crtcList;
	std::vector<DrmPlane> planeList;

	unsigned int width=1920; //TODO:: move to crtc
	unsigned int height=1080; //TODO:: move to crtc
	//uint32_t vRefresh = 0;
	uint32_t stride=0;

	uint32_t findCrtc(DrmConnector &conn);
	int hasDumbBuff();

	int setupDevice();
	int geModeRange(AVAL_VIDEO_SIZE_T &minSize, AVAL_VIDEO_SIZE_T &maxSize);

	DriDevice(){}

	~DriDevice();

	int setActiveMode(DrmCrtc&, const uint32_t width, const uint32_t vRefreshheight, const uint32_t vRefresh=0);

	friend DRIElements;
};

typedef enum
{
	SET_PLANE_FB_T = 0xff01,
	SET_Z_ORDER_T = 0xff02,
	SET_SCALING_T = 0xff03
} PLANE_PROPS_T;



class DRIElements
{
public:

	DRIElements(AVAL_VIDEO_SIZE_T defResolution,
	            std::function<void(AVAL_VIDEO_SIZE_T, AVAL_VIDEO_SIZE_T)>);
	virtual ~DRIElements();

	std::string mPrimaryDev;
	int changeMode(uint32_t width, uint32_t height, uint32_t vRefresh =0);
	std::unordered_map<std::string, DriDevice> mDeviceList;
	std::vector<uint32_t> getPlanes();
	bool setPlane(unsigned int planeId, unsigned int fbId, uint32_t crtc_x, uint32_t  crtc_y, uint32_t  crtc_w, uint32_t  crtc_h,
	              uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h);
	std::vector<AVAL_VIDEO_SIZE_T> getSupportedModes();
	bool setPlaneProperties( PLANE_PROPS_T propType, uint planeId,uint64_t value);

	uint32_t getPlaneBase();

private:

	class UDev
	{
		struct udev* udev;
		struct udev_enumerate* enumerate;
		struct udev_list_entry* devices;
		struct udev_monitor* mon;
		int fd;
		std::function<void(std::string)> updateFun;
	public:
		UDev(std::function<void(std::string)>);
		static gboolean pollDRIDevices(gpointer userData);
		std::vector<std::string> getDeviceList();
	};

	void setupDevicePolling();
	void updateDevice(std::string name);
	void loadResources();


	guint mTimeOutHandle;
	UDev *mUDev = nullptr;
	friend DriDevice;

	std::function<void(AVAL_VIDEO_SIZE_T, AVAL_VIDEO_SIZE_T)> mAvalCallBack;
	AVAL_VIDEO_SIZE_T mInitialMode; //Set from device_capability config file.
	AVAL_VIDEO_SIZE_T mConfiguredMode; //Updated by changeMode or luna command.
};
