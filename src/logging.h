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
#include <exception>
#include <sstream>
#include <memory>
#include <PmLogLib.h>

extern PmLogContext avalLogContext;
#define LOG_CRITICAL(msgid, kvcount, ...) \
	PmLogCritical(avalLogContext, msgid, kvcount, ##__VA_ARGS__)

#define LOG_ERROR(msgid, kvcount, ...) \
	PmLogError(avalLogContext, msgid, kvcount,##__VA_ARGS__)

#define LOG_WARNING(msgid, kvcount, ...) \
	PmLogWarning(avalLogContext, msgid, kvcount, ##__VA_ARGS__)

#define LOG_INFO(msgid, kvcount, ...) \
	PmLogInfo(avalLogContext, msgid, kvcount, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...) \
	PmLogDebug(avalLogContext, "%s:%s() " fmt, __FILE__, __FUNCTION__, ##__VA_ARGS__)

#define LOG_ESCAPED_ERRMSG(msgid, errmsg) \
    do { \
    gchar *escaped_errtext = g_strescape(errmsg, NULL); \
     LOG_ERROR(msgid, 1, PMLOGKS("Error", escaped_errtext), ""); \
    g_free(escaped_errtext); \
    } while(0)

static std::string string_format_valist(const std::string& fmt_str, va_list ap)
{
	size_t n = fmt_str.size() * 2;
	std::unique_ptr<char[]> formatted(new char[n]);
	va_list apCopy;
	va_copy(apCopy, ap);

	int final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
	if (final_n < 0 || final_n >= (int)n)
	{
			/* There was not enough space, retry */
			/* MS implements < 0 as not large enough */
			n = (size_t) (abs(final_n) + 1);

			formatted.reset(new char[n]);
			vsnprintf(&formatted[0], n, fmt_str.c_str(), apCopy);
			}
	 va_end(apCopy);

	return std::string(formatted.get());
}

class FatalException : public std::exception
{
public:
	FatalException(const char *file, int line, const char* format, ...)
	{
		std::stringstream s;
		va_list args;
		va_start(args, format);
		s << file << ":" << line << ": " << string_format_valist(format, args);
		va_end(args);
		mMessage = s.str();
		LOG_ERROR("FATAL_ERROR", 0, "%s", mMessage.c_str());
		va_end(args);
	}
	const char* what() const noexcept override
	{
		return mMessage.c_str();
	}

private:
	std::string mMessage;
};

#define THROW_FATAL_EXCEPTION(...)  throw FatalException(__FILE__, __LINE__, __VA_ARGS__)

#define MSGID_SET_VOLUME_ERROR           "SET_VOLUME_ERROR"
#define MSGID_SET_MUTE_ERROR             "SET_MUTE_ERROR"

//config file releated
#define MSGID_LOAD_CONFIG                "LOAD_CONFIG"
#define MSGID_CONFIG_FILE_READ_FAILED    "CONFIG_FILE_READ_FAILED"
#define MSGID_CONFFILE_MISCONFIGURED     "CONFIG_FILE_MISCONFIGURED"

//setup drm errors
#define MSGID_BUFFER_CREATION_FAILED     "BUFFER_CREATION_FAILED"
#define MSGID_FB_CREATION_FAILED         "FB_CREATION_FAILED"
#define MSGID_INVALID_DISPLAY_MODE       "INVALID_DISPLAY_MODE"
#define MSGID_DISPLAY_NOT_CONNECTED      "MSGID_DISPLAY_NOT_CONNECTED"
#define MSGID_DEVICE_STATUS              "MSGID_DEVICE_STATUS"
#define MSGID_UDEV_ERROR                 "MSGID_UDEV__ERROR"
#define MSGID_DEVICE_ERROR               "MSGID_DEVICE_ERROR"
#define MSGID_DRM_MODESET_ERROR          "MSGID_DRM_MODESET_ERROR"

//video errors
#define MSGID_VIDEO_CONNECT_FAILED       "VIDEO_CONNECT_FAILED"
#define MSGID_VIDEO_DISCONNECT_FAILED    "VIDEO_DISCONNECT_FAILED"
#define MSGID_VIDEO_SCALING_FAILED       "VIDEO_SCALING_FAILED"
#define MSGID_SET_ZORDER_FAILED          "SET_ZORDER_FAILED"
#define MSGID_VIDEO_BLANKING_FAILED      "VIDEO_BLANKING_FAILED"
#define MSGID_VIDEO_UNBLANKING_FAILED    "VIDEO_UNBLANKING_FAILED"
#define MSGID_DRM_SET_PLANE_FAILED       "DRM_SET_PLANE_FAILED"
#define MSGID_DRM_SET_PROP_FAILED        "MSGID_DRM_SET_PROP_FAILED"
#define MSGID_MODE_CHANGE_FAILED          "MODE_CHANGE_FAILED"