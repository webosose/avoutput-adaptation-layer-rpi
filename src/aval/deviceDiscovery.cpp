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
#include <libudev.h>
#include <sstream>
#include "logging.h"


static const char* const DEVICE_SUBSYSTEM = "drm";
static constexpr uint32_t DISPLAY_PLUGGED_POLL_TIMEOUT = 250;


DRIElements::UDev::UDev(std::function<void(std::string)> fn):updateFun(fn)
{
	udev = udev_new();
	if (!udev)
	{
		THROW_FATAL_EXCEPTION("Unable to open udev");
	}
	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(mon, DEVICE_SUBSYSTEM, NULL);
	udev_monitor_enable_receiving(mon);
	fd = udev_monitor_get_fd(mon);
}

gboolean DRIElements::UDev::pollDRIDevices(gpointer userData)
{
	fd_set fds;
	struct timeval tv;
	int ret;
	UDev* uDevMonitor = static_cast<UDev*>(userData);
	FD_ZERO(&fds);
	FD_SET(uDevMonitor->fd, &fds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	ret = select(uDevMonitor->fd + 1, &fds, NULL, NULL, &tv);
	if (ret > 0 && FD_ISSET(uDevMonitor->fd, &fds))
	{
		struct udev_device* dev = udev_monitor_receive_device(uDevMonitor->mon);
		if (dev)
		{
			std::stringstream ss;
			ss << "Got Device\n" ;
			std::string node = udev_device_get_devnode(dev);
			ss << "\n   Node:  "<< node,
			ss << "\n   Subsystem: " << udev_device_get_subsystem(dev);
			ss << "\n   Devtype: " << udev_device_get_devtype(dev);
			ss << "\n   Action: "<< udev_device_get_action(dev);
			udev_device_unref(dev);
			LOG_INFO(MSGID_DEVICE_STATUS,0,ss.str().c_str());
			uDevMonitor->updateFun(node);
		} else
		{
			LOG_ERROR(MSGID_UDEV_ERROR, 0, "No Device from receive_device. An error occured");
		}
	}
	return true;
}

std::vector<std::string> DRIElements::UDev::getDeviceList()
{
	std::vector<std::string> deviceNodes;

	struct udev_enumerate *enumerate = nullptr;
	struct udev_list_entry *udevices = nullptr, *udev_list_entry  = nullptr;
	struct udev_device *dev = nullptr;

	enumerate = udev_enumerate_new(udev);

	udev_enumerate_add_match_subsystem(enumerate, DEVICE_SUBSYSTEM);

	udev_enumerate_scan_devices(enumerate);

	udevices = udev_enumerate_get_list_entry(enumerate);
	udev_list_entry_foreach(udev_list_entry, udevices) {
		const char *path = udev_list_entry_get_name(udev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);
		const char *node = udev_device_get_devnode(dev);
		if (node)
		{
			deviceNodes.push_back(std::string(node));
			LOG_INFO(MSGID_DEVICE_STATUS,0,"Found device name %s", node);
		}
		udev_device_unref(dev);
	}
	udev_enumerate_unref(enumerate);
	return deviceNodes;

}

void DRIElements::setupDevicePolling()
{
	mTimeOutHandle = g_timeout_add(DISPLAY_PLUGGED_POLL_TIMEOUT,
	                               DRIElements::UDev::pollDRIDevices, mUDev);
}