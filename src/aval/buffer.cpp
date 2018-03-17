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


/*
 * DRM based mode setting test program
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */




#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <iostream>

#include "drm.h"
#include "drm_fourcc.h"

#include "libdrm_macros.h"
#include "xf86drm.h"
#include "buffers.h"
#include "logging.h"


/* -----------------------------------------------------------------------------
 * Buffers management
 */

static struct bo *
bo_create_dumb(int fd, unsigned int width, unsigned int height, unsigned int bpp)
{
	struct drm_mode_create_dumb arg;
	struct bo *bo;
	int ret;

	bo = (struct bo*)calloc(1, sizeof(*bo));
	if (bo == NULL) {
		fprintf(stderr, "failed to allocate buffer object\n");
		return NULL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.bpp = bpp;
	arg.width = width;
	arg.height = height;

	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg);
	if (ret) {
		fprintf(stderr, "failed to create dumb buffer: %s\n",
		        strerror(errno));
		free(bo);
		return NULL;
	}

	bo->fd = fd;
	bo->handle = arg.handle;
	bo->size = arg.size;
	bo->pitch = arg.pitch;

	return bo;
}

int bo_map(struct bo *bo, void **out)
{
	struct drm_mode_map_dumb arg;
	void *map;
	int ret;
	if (!bo)
	{
		fprintf(stderr, " \n *** bo is null \n");
		return -1;
	}
	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->handle;

	ret = drmIoctl(bo->fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
	if (ret)
	{
		LOG_ERROR(MSGID_DRM_MODESET_ERROR,0, "failed to map dumb buffer -DRM_IOCTL_MODE_MAP_DUMB: %d",ret);
		return ret;
	}

	map = drm_mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
	               bo->fd, arg.offset);
	if (map == MAP_FAILED)
	{
		LOG_ERROR(MSGID_DRM_MODESET_ERROR,0, "failed to map buffer- drm_mmap %d",ret);
		return -EINVAL;
	}

	bo->ptr = map;
	*out = map;

	return 0;
}

void bo_unmap(struct bo *bo)
{
	if (!bo->ptr)
		return;

	drm_munmap(bo->ptr, bo->size);
	bo->ptr = NULL;
}

struct bo *
bo_create(int fd, unsigned int format,
          unsigned int width, unsigned int height,
          unsigned int handles[4], unsigned int pitches[4],
          unsigned int offsets[4]) // enum util_fill_pattern pattern
{
	unsigned int virtual_height;
	struct bo *bo;
	unsigned int bpp;


	switch (format) {
		case DRM_FORMAT_NV12:
		case DRM_FORMAT_NV21:
		case DRM_FORMAT_NV16:
		case DRM_FORMAT_NV61:
		case DRM_FORMAT_YUV420:
		case DRM_FORMAT_YVU420:
			bpp = 8;
			break;

		case DRM_FORMAT_ARGB4444:
		case DRM_FORMAT_XRGB4444:
		case DRM_FORMAT_ABGR4444:
		case DRM_FORMAT_XBGR4444:
		case DRM_FORMAT_RGBA4444:
		case DRM_FORMAT_RGBX4444:
		case DRM_FORMAT_BGRA4444:
		case DRM_FORMAT_BGRX4444:
		case DRM_FORMAT_ARGB1555:
		case DRM_FORMAT_XRGB1555:
		case DRM_FORMAT_ABGR1555:
		case DRM_FORMAT_XBGR1555:
		case DRM_FORMAT_RGBA5551:
		case DRM_FORMAT_RGBX5551:
		case DRM_FORMAT_BGRA5551:
		case DRM_FORMAT_BGRX5551:
		case DRM_FORMAT_RGB565:
		case DRM_FORMAT_BGR565:
		case DRM_FORMAT_UYVY:
		case DRM_FORMAT_VYUY:
		case DRM_FORMAT_YUYV:
		case DRM_FORMAT_YVYU:
			bpp = 16;
			break;

		case DRM_FORMAT_BGR888:
		case DRM_FORMAT_RGB888:
			bpp = 24;
			break;

		case DRM_FORMAT_ARGB8888:
		case DRM_FORMAT_XRGB8888:
		case DRM_FORMAT_ABGR8888:
		case DRM_FORMAT_XBGR8888:
		case DRM_FORMAT_RGBA8888:
		case DRM_FORMAT_RGBX8888:
		case DRM_FORMAT_BGRA8888:
		case DRM_FORMAT_BGRX8888:
		case DRM_FORMAT_ARGB2101010:
		case DRM_FORMAT_XRGB2101010:
		case DRM_FORMAT_ABGR2101010:
		case DRM_FORMAT_XBGR2101010:
		case DRM_FORMAT_RGBA1010102:
		case DRM_FORMAT_RGBX1010102:
		case DRM_FORMAT_BGRA1010102:
		case DRM_FORMAT_BGRX1010102:
			bpp = 32;
			break;

		default:
			fprintf(stderr, "unsupported format 0x%08x\n",  format);
			return NULL;
	}

	switch (format) {
		case DRM_FORMAT_NV12:
		case DRM_FORMAT_NV21:
		case DRM_FORMAT_YUV420:
		case DRM_FORMAT_YVU420:
			virtual_height = height * 3 / 2;
			break;

		case DRM_FORMAT_NV16:
		case DRM_FORMAT_NV61:
			virtual_height = height * 2;
			break;

		default:
			virtual_height = height;
			break;
	}

	bo = bo_create_dumb(fd, width, virtual_height, bpp);
	if (!bo)
		return NULL;

	//Get handles and pitches required by AddFb
	switch (format) {
		case DRM_FORMAT_UYVY:
		case DRM_FORMAT_VYUY:
		case DRM_FORMAT_YUYV:
		case DRM_FORMAT_YVYU:
			offsets[0] = 0;
			handles[0] = bo->handle;
			pitches[0] = bo->pitch;
			break;

		case DRM_FORMAT_NV12:
		case DRM_FORMAT_NV21:
		case DRM_FORMAT_NV16:
		case DRM_FORMAT_NV61:
			offsets[0] = 0;
			handles[0] = bo->handle;
			pitches[0] = bo->pitch;
			pitches[1] = pitches[0];
			offsets[1] = pitches[0] * height;
			handles[1] = bo->handle;

			break;

		case DRM_FORMAT_YUV420:
		case DRM_FORMAT_YVU420:
			offsets[0] = 0;
			handles[0] = bo->handle;
			pitches[0] = bo->pitch;
			pitches[1] = pitches[0] / 2;
			offsets[1] = pitches[0] * height;
			handles[1] = bo->handle;
			pitches[2] = pitches[1];
			offsets[2] = offsets[1] + pitches[1] * height / 2;
			handles[2] = bo->handle;
			break;

		case DRM_FORMAT_ARGB4444:
		case DRM_FORMAT_XRGB4444:
		case DRM_FORMAT_ABGR4444:
		case DRM_FORMAT_XBGR4444:
		case DRM_FORMAT_RGBA4444:
		case DRM_FORMAT_RGBX4444:
		case DRM_FORMAT_BGRA4444:
		case DRM_FORMAT_BGRX4444:
		case DRM_FORMAT_ARGB1555:
		case DRM_FORMAT_XRGB1555:
		case DRM_FORMAT_ABGR1555:
		case DRM_FORMAT_XBGR1555:
		case DRM_FORMAT_RGBA5551:
		case DRM_FORMAT_RGBX5551:
		case DRM_FORMAT_BGRA5551:
		case DRM_FORMAT_BGRX5551:
		case DRM_FORMAT_RGB565:
		case DRM_FORMAT_BGR565:
		case DRM_FORMAT_BGR888:
		case DRM_FORMAT_RGB888:
		case DRM_FORMAT_ARGB8888:
		case DRM_FORMAT_XRGB8888:
		case DRM_FORMAT_ABGR8888:
		case DRM_FORMAT_XBGR8888:
		case DRM_FORMAT_RGBA8888:
		case DRM_FORMAT_RGBX8888:
		case DRM_FORMAT_BGRA8888:
		case DRM_FORMAT_BGRX8888:
		case DRM_FORMAT_ARGB2101010:
		case DRM_FORMAT_XRGB2101010:
		case DRM_FORMAT_ABGR2101010:
		case DRM_FORMAT_XBGR2101010:
		case DRM_FORMAT_RGBA1010102:
		case DRM_FORMAT_RGBX1010102:
		case DRM_FORMAT_BGRA1010102:
		case DRM_FORMAT_BGRX1010102:
			offsets[0] = 0;
			handles[0] = bo->handle;
			pitches[0] = bo->pitch;
		break;
	}


	return bo;
}

void bo_destroy(struct bo *bo)
{
	struct drm_mode_destroy_dumb arg;
	int ret;
	if(!bo) return;
	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->handle;

	ret = drmIoctl(bo->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);
	if (ret)
	{
		LOG_ERROR(MSGID_DRM_MODESET_ERROR, 0, "failed to destroy dumb buffer: %s", strerror(errno));
	}

	free(bo);
}
