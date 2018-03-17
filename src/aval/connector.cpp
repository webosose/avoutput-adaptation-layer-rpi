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

#include "driElements.h"
#include "logging.h"

extern const char *util_lookup_connector_type_name(unsigned int type);

DrmConnector::DrmConnector(int drmModulefd, drmModeConnectorPtr pConnector)
{
	if (!pConnector || drmModulefd <= 0 )
	{
		THROW_FATAL_EXCEPTION("Invalid connector ");
	}
	mConnectorPtr = pConnector;
	mDrmModulefd = drmModulefd;
	mName = util_lookup_connector_type_name(pConnector->connector_type);
}


bool DrmConnector::isPlugged()
{
	//Reset connectorPtr
	uint32_t conn_id = 0;
	if (!mConnectorPtr)
	{
		THROW_FATAL_EXCEPTION("Initialization error -found null connector");
	}
	else
	{
		conn_id = mConnectorPtr->connector_id;
		drmModeFreeConnector(mConnectorPtr);
	}
	mConnectorPtr = drmModeGetConnector(mDrmModulefd,conn_id);
	return (mConnectorPtr->connection == DRM_MODE_CONNECTED && mConnectorPtr->count_modes !=0);
}

bool DrmConnector::getModeRange(DrmDisplayMode &min, DrmDisplayMode &max)
{
	if (!isPlugged())
		return false;
	//TODO::expecting sorted modes. confirm that it will be always sorted or fixit
	max.mModeInfoPtr = &mConnectorPtr->modes[mConnectorPtr->count_modes-1];
	min.mModeInfoPtr = &mConnectorPtr->modes[0];
	return true;
}

DrmDisplayMode DrmConnector::getMode(const std::string mode_name, const uint32_t vRefresh)
{
	for (int i = 0; i < mConnectorPtr->count_modes; i++)
	{
		drmModeModeInfo* mode = &mConnectorPtr->modes[i];
		if (mode_name == mode->name)
		{
			/* If the vertical refresh frequency is not specified then return the
			 * first mode that match with the name. Else, return the mode that match
			 * the name and the specified vertical refresh frequency.
			 */
			if (vRefresh == 0 || mode->vrefresh == vRefresh)
				return DrmDisplayMode(mode);
		}
	}
	return DrmDisplayMode();
}

bool DrmConnector::isModeSupported(std::string mode_name, const uint32_t vRefresh)
{
	for (int i = 0; i < mConnectorPtr->count_modes; i++) {
		drmModeModeInfo *mode = &mConnectorPtr->modes[i];
		if (mode_name == mode->name) {
			/* If the vertical refresh frequency is not specified then return the
			 * first mode that match with the name. Else, return the mode that match
			 * the name and the specified vertical refresh frequency.
			 */
			if (vRefresh == 0 || mode->vrefresh == vRefresh)
				return true;
		}
	}
	return false;
}



Edid DrmConnector::getEdid()
{
	int j;
	drmModePropertyPtr props;

	for (j=0; j < mConnectorPtr->count_props; j++)
	{
		props = drmModeGetProperty(mDrmModulefd, mConnectorPtr->props[j]);

		if (props) {

			if (std::string(props->name) == "EDID")
			{
				if (props->flags & DRM_MODE_PROP_BLOB) {
					drmModePropertyBlobPtr blob;

					blob = drmModeGetPropertyBlob(mDrmModulefd, mConnectorPtr->prop_values[j]);
					if (blob) {
						Edid edid = Edid((unsigned char*) blob->data, blob->length);
						drmModeFreePropertyBlob(blob);
						drmModeFreeProperty(props);
						return edid;
					} else {
						THROW_FATAL_EXCEPTION("error getting edid blob %llu" , mConnectorPtr->prop_values[j]);
					}
				}
			}
			//TODO:: DPMS property and others.
			drmModeFreeProperty(props);
		}
	}
	return	Edid();
}

std::vector<AVAL_VIDEO_SIZE_T> DrmConnector::getSupportedModes()
{
	std::vector<AVAL_VIDEO_SIZE_T> modeList;
	if (mConnectorPtr->count_modes)
	{
		for (int j = 0; j < mConnectorPtr->count_modes; j++)
		{
			auto mode =  &mConnectorPtr->modes[j];
			AVAL_VIDEO_SIZE_T dim;
			dim.w= mode->hdisplay;
			dim.h = mode->vdisplay;
			modeList.push_back(dim);
		}
	}
	return modeList;
}
