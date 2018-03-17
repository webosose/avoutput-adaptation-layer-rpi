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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xf86drm.h"
#include "xf86drmMode.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct type_name {
	unsigned int type;
	const char *name;
};

static const char *util_lookup_type_name(unsigned int type,
                                         const struct type_name *table,
                                         unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++)
		if (table[i].type == type)
			return table[i].name;

	return NULL;
}

static const struct type_name encoder_type_names[] = {
		{ DRM_MODE_ENCODER_NONE, "none" },
		{ DRM_MODE_ENCODER_DAC, "DAC" },
		{ DRM_MODE_ENCODER_TMDS, "TMDS" },
		{ DRM_MODE_ENCODER_LVDS, "LVDS" },
		{ DRM_MODE_ENCODER_TVDAC, "TVDAC" },
		{ DRM_MODE_ENCODER_VIRTUAL, "Virtual" },
		{ DRM_MODE_ENCODER_DSI, "DSI" },
		{ DRM_MODE_ENCODER_DPMST, "DPMST" },
};

const char *util_lookup_encoder_type_name(unsigned int type)
{
	return util_lookup_type_name(type, encoder_type_names,
	                             ARRAY_SIZE(encoder_type_names));
}

static const struct type_name connector_status_names[] = {
		{ DRM_MODE_CONNECTED, "connected" },
		{ DRM_MODE_DISCONNECTED, "disconnected" },
		{ DRM_MODE_UNKNOWNCONNECTION, "unknown" },
};


const char *util_lookup_connector_status_name(unsigned int status)
{
	return util_lookup_type_name(status, connector_status_names,
	                             ARRAY_SIZE(connector_status_names));
}

static const struct type_name connector_type_names[] = {
		{ DRM_MODE_CONNECTOR_Unknown, "unknown" },
		{ DRM_MODE_CONNECTOR_VGA, "VGA" },
		{ DRM_MODE_CONNECTOR_DVII, "DVI-I" },
		{ DRM_MODE_CONNECTOR_DVID, "DVI-D" },
		{ DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
		{ DRM_MODE_CONNECTOR_Composite, "composite" },
		{ DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
		{ DRM_MODE_CONNECTOR_LVDS, "LVDS" },
		{ DRM_MODE_CONNECTOR_Component, "component" },
		{ DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
		{ DRM_MODE_CONNECTOR_DisplayPort, "DP" },
		{ DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
		{ DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
		{ DRM_MODE_CONNECTOR_TV, "TV" },
		{ DRM_MODE_CONNECTOR_eDP, "eDP" },
		{ DRM_MODE_CONNECTOR_VIRTUAL, "Virtual" },
		{ DRM_MODE_CONNECTOR_DSI, "DSI" },
};

const char *util_lookup_connector_type_name(unsigned int type)
{
	return util_lookup_type_name(type, connector_type_names,
	                             ARRAY_SIZE(connector_type_names));
}


/********************
*Modes util
*/
static std::vector<std::string>  mode_type_names {
		"builtin",
		"clock_c",
		"crtc_c",
		"preferred",
		"default",
		"userdef",
		"driver"
};
static std::vector<std::string>  mode_flag_names{
		"phsync",
		"nhsync",
		"pvsync",
		"nvsync",
		"interlace",
		"dblscan",
		"csync",
		"pcsync",
		"ncsync",
		"hskew",
		"bcast",
		"pixmux",
		"dblclk",
		"clkdiv2"
};

static std::string mode_flag_str(int flags)
{
	std::string ret;
	unsigned int i;
	std::string sep = " ";
	for (i = 0; i < mode_flag_names.size(); i++) {
		if (flags & (1 << i)) {
			ret += sep   + mode_flag_names[i];
			sep = ", ";
		}
	}
	return ret;
}

std::string mode_type_str(int type)
{
	std::string ret;
	unsigned int i;
	const char *sep = "";
	for (i = 0; i < mode_type_names.size(); i++) {
		if (type & (1 << i)) {
			ret += sep + mode_type_names[i];
			sep = ", ";
		}
	}
	return ret;
}

std::ostream& operator<< (std::ostream &os, const DrmDisplayMode &dm)
{
	//TODO::jsonify
	os << "name refresh (Hz) hdisp hss hse htot vdisp vss vse vtot";
	os <<   dm.mModeInfoPtr->name << " " <<\
			        dm.mModeInfoPtr->vrefresh << " " <<\
					dm.mModeInfoPtr->hdisplay << " " <<\
					dm.mModeInfoPtr->hsync_start<< " " <<\
			      dm.mModeInfoPtr->hsync_end<< " " << \
					dm.mModeInfoPtr->htotal<< " " << \
					dm.mModeInfoPtr->vdisplay<< " " << \
					dm.mModeInfoPtr->vsync_start<< " " << \
					dm.mModeInfoPtr->vsync_end<< " " << \
					dm.mModeInfoPtr->vtotal<< " " << \
					dm.mModeInfoPtr->clock << " " ;

	os << " flags: " << mode_flag_str(dm.mModeInfoPtr->flags);
	os << "; type: " << mode_type_str(dm.mModeInfoPtr->type);
	os << std::endl;
	return os;
}


std::ostream& operator<< (std::ostream &os, const DrmPlane &dm)
{
	os << "Plane:\n";
	os << "id\tcrtc\tfb\tCRTC x,y\tx,y\tgamma size\tpossible crtcs\n";

	auto ovr = dm.mDrmPlane;
	os << ovr->plane_id << "\t" << ovr->crtc_id<< "\t" << ovr->fb_id
	   << "\t" << ovr->crtc_x << "\t" << ovr->crtc_y << "\t" << ovr->x << "\t" << ovr->y
	   << "\t" << ovr->gamma_size<< "x" << ovr->possible_crtcs;

	if (!ovr->count_formats)
		return os;

	os <<"  formats:";
	for (size_t j = 0; j < ovr->count_formats; j++)
		os << " "<< (char *)&ovr->formats[j];
	os << "\n";


	return os;
}

void dumpProperties ( std::ostream &os, drmModePropertyPtr prop,
                      uint32_t prop_id, uint64_t value)
{
	os << "TODO Dump props";
}
