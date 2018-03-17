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

#ifndef UTIL_FORMAT_H
#define UTIL_FORMAT_H

struct util_color_component {
	unsigned int length;
	unsigned int offset;
};

struct util_rgb_info {
	struct util_color_component red;
	struct util_color_component green;
	struct util_color_component blue;
	struct util_color_component alpha;
};

enum util_yuv_order {
	YUV_YCbCr = 1,
	YUV_YCrCb = 2,
	YUV_YC = 4,
	YUV_CY = 8,
};

struct util_yuv_info {
	enum util_yuv_order order;
	unsigned int xsub;
	unsigned int ysub;
	unsigned int chroma_stride;
};

struct util_format_info {
	uint32_t format;
	const char *name;
	const struct util_rgb_info rgb;
	const struct util_yuv_info yuv;
};

struct color_rgb24 {
	unsigned int value:24;
} __attribute__((__packed__));

struct color_yuv {
	unsigned char y;
	unsigned char u;
	unsigned char v;
};
uint32_t util_format_fourcc(const char *name);
const struct util_format_info *util_format_info_find(uint32_t format);


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define MAKE_YUV_601_Y(r, g, b) \
	((( 66 * (r) + 129 * (g) +  25 * (b) + 128) >> 8) + 16)
#define MAKE_YUV_601_U(r, g, b) \
	(((-38 * (r) -  74 * (g) + 112 * (b) + 128) >> 8) + 128)
#define MAKE_YUV_601_V(r, g, b) \
	(((112 * (r) -  94 * (g) -  18 * (b) + 128) >> 8) + 128)

#define MAKE_YUV_601(r, g, b) \
	{ y : MAKE_YUV_601_Y(r, g, b), \
	  u : MAKE_YUV_601_U(r, g, b), \
	  v : MAKE_YUV_601_V(r, g, b) }

#define MAKE_RGBA(rgb, r, g, b, a) \
	((((r) >> (8 - (rgb)->red.length)) << (rgb)->red.offset) | \
	 (((g) >> (8 - (rgb)->green.length)) << (rgb)->green.offset) | \
	 (((b) >> (8 - (rgb)->blue.length)) << (rgb)->blue.offset) | \
	 (((a) >> (8 - (rgb)->alpha.length)) << (rgb)->alpha.offset))

#define MAKE_RGB24(rgb, r, g, b) \
	{ value : MAKE_RGBA(rgb, r, g, b, 0) }

#define MAKE_RGB_INFO(rl, ro, gl, go, bl, bo, al, ao) \
	rgb : { {(rl), (ro) }, { (gl), (go) }, { (bl), (bo) }, { (al), (ao) }}, \
    yuv : { (enum util_yuv_order)0, 0, 0, 0}


#define MAKE_YUV_INFO(order, xsub, ysub, chroma_stride) \
	rgb : { {0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }}, \
	yuv : { (enum util_yuv_order) (order), (xsub), (ysub), (chroma_stride) }

const struct util_format_info format_info[] = {
		/* YUV packed */
		{ format : DRM_FORMAT_UYVY, name : "UYVY", MAKE_YUV_INFO(YUV_YCbCr | YUV_CY, 2, 2, 2) },
		{ format : DRM_FORMAT_VYUY, name : "VYUY", MAKE_YUV_INFO(YUV_YCrCb | YUV_CY, 2, 2, 2) },
		{ format : DRM_FORMAT_YUYV, name : "YUYV", MAKE_YUV_INFO(YUV_YCbCr | YUV_YC, 2, 2, 2) },
		{ format : DRM_FORMAT_YVYU, name : "YVYU", MAKE_YUV_INFO(YUV_YCrCb | YUV_YC, 2, 2, 2) },
		/* YUV semi-planar */
		{ format : DRM_FORMAT_NV12, name : "NV12", MAKE_YUV_INFO(YUV_YCbCr, 2, 2, 2) },
		{ format : DRM_FORMAT_NV21, name : "NV21", MAKE_YUV_INFO(YUV_YCrCb, 2, 2, 2) },
		{ format : DRM_FORMAT_NV16, name : "NV16", MAKE_YUV_INFO(YUV_YCbCr, 2, 1, 2) },
		{ format : DRM_FORMAT_NV61, name : "NV61", MAKE_YUV_INFO(YUV_YCrCb, 2, 1, 2) },
		/* YUV planar */
		{ format : DRM_FORMAT_YUV420, name : "YU12", MAKE_YUV_INFO(YUV_YCbCr, 2, 2, 1) },
		{ format : DRM_FORMAT_YVU420, name : "YV12", MAKE_YUV_INFO(YUV_YCrCb, 2, 2, 1) },
		/* RGB16 */
		{ format : DRM_FORMAT_ARGB4444, name : "AR12", MAKE_RGB_INFO(4, 8, 4, 4, 4, 0, 4, 12) },
		{ format : DRM_FORMAT_XRGB4444, name : "XR12", MAKE_RGB_INFO(4, 8, 4, 4, 4, 0, 0, 0) },
		{ format : DRM_FORMAT_ABGR4444, name : "AB12", MAKE_RGB_INFO(4, 0, 4, 4, 4, 8, 4, 12) },
		{ format : DRM_FORMAT_XBGR4444, name : "XB12", MAKE_RGB_INFO(4, 0, 4, 4, 4, 8, 0, 0) },
		{ format : DRM_FORMAT_RGBA4444, name : "RA12", MAKE_RGB_INFO(4, 12, 4, 8, 4, 4, 4, 0) },
		{ format : DRM_FORMAT_RGBX4444, name : "RX12", MAKE_RGB_INFO(4, 12, 4, 8, 4, 4, 0, 0) },
		{ format : DRM_FORMAT_BGRA4444, name : "BA12", MAKE_RGB_INFO(4, 4, 4, 8, 4, 12, 4, 0) },
		{ format : DRM_FORMAT_BGRX4444, name : "BX12", MAKE_RGB_INFO(4, 4, 4, 8, 4, 12, 0, 0) },
		{ format : DRM_FORMAT_ARGB1555, name : "AR15", MAKE_RGB_INFO(5, 10, 5, 5, 5, 0, 1, 15) },
		{ format : DRM_FORMAT_XRGB1555, name : "XR15", MAKE_RGB_INFO(5, 10, 5, 5, 5, 0, 0, 0) },
		{ format : DRM_FORMAT_ABGR1555, name : "AB15", MAKE_RGB_INFO(5, 0, 5, 5, 5, 10, 1, 15) },
		{ format : DRM_FORMAT_XBGR1555, name : "XB15", MAKE_RGB_INFO(5, 0, 5, 5, 5, 10, 0, 0) },
		{ format : DRM_FORMAT_RGBA5551, name : "RA15", MAKE_RGB_INFO(5, 11, 5, 6, 5, 1, 1, 0) },
		{ format : DRM_FORMAT_RGBX5551, name : "RX15", MAKE_RGB_INFO(5, 11, 5, 6, 5, 1, 0, 0) },
		{ format : DRM_FORMAT_BGRA5551, name : "BA15", MAKE_RGB_INFO(5, 1, 5, 6, 5, 11, 1, 0) },
		{ format : DRM_FORMAT_BGRX5551, name : "BX15", MAKE_RGB_INFO(5, 1, 5, 6, 5, 11, 0, 0) },
		{ format : DRM_FORMAT_RGB565, name : "RG16", MAKE_RGB_INFO(5, 11, 6, 5, 5, 0, 0, 0) },
		{ format : DRM_FORMAT_BGR565, name : "BG16", MAKE_RGB_INFO(5, 0, 6, 5, 5, 11, 0, 0) },
		/* RGB24 */
		{ format : DRM_FORMAT_BGR888, name : "BG24", MAKE_RGB_INFO(8, 0, 8, 8, 8, 16, 0, 0) },
		{ format : DRM_FORMAT_RGB888, name : "RG24", MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 0, 0) },
		/* RGB32 */
		{ format : DRM_FORMAT_ARGB8888, name : "AR24", MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 8, 24) },
		{ format : DRM_FORMAT_XRGB8888, name : "XR24", MAKE_RGB_INFO(8, 16, 8, 8, 8, 0, 0, 0) },
		{ format : DRM_FORMAT_ABGR8888, name : "AB24", MAKE_RGB_INFO(8, 0, 8, 8, 8, 16, 8, 24) },
		{ format : DRM_FORMAT_XBGR8888, name : "XB24", MAKE_RGB_INFO(8, 0, 8, 8, 8, 16, 0, 0) },
		{ format : DRM_FORMAT_RGBA8888, name : "RA24", MAKE_RGB_INFO(8, 24, 8, 16, 8, 8, 8, 0) },
		{ format : DRM_FORMAT_RGBX8888, name : "RX24", MAKE_RGB_INFO(8, 24, 8, 16, 8, 8, 0, 0) },
		{ format : DRM_FORMAT_BGRA8888, name : "BA24", MAKE_RGB_INFO(8, 8, 8, 16, 8, 24, 8, 0) },
		{ format : DRM_FORMAT_BGRX8888, name : "BX24", MAKE_RGB_INFO(8, 8, 8, 16, 8, 24, 0, 0) },
		{ format : DRM_FORMAT_ARGB2101010, name : "AR30", MAKE_RGB_INFO(10, 20, 10, 10, 10, 0, 2, 30) },
		{ format : DRM_FORMAT_XRGB2101010, name : "XR30", MAKE_RGB_INFO(10, 20, 10, 10, 10, 0, 0, 0) },
		{ format : DRM_FORMAT_ABGR2101010, name : "AB30", MAKE_RGB_INFO(10, 0, 10, 10, 10, 20, 2, 30) },
		{ format : DRM_FORMAT_XBGR2101010, name : "XB30", MAKE_RGB_INFO(10, 0, 10, 10, 10, 20, 0, 0) },
		{ format : DRM_FORMAT_RGBA1010102, name : "RA30", MAKE_RGB_INFO(10, 22, 10, 12, 10, 2, 2, 0) },
		{ format : DRM_FORMAT_RGBX1010102, name : "RX30", MAKE_RGB_INFO(10, 22, 10, 12, 10, 2, 0, 0) },
		{ format : DRM_FORMAT_BGRA1010102, name : "BA30", MAKE_RGB_INFO(10, 2, 10, 12, 10, 22, 2, 0) },
		{ format : DRM_FORMAT_BGRX1010102, name : "BX30", MAKE_RGB_INFO(10, 2, 10, 12, 10, 22, 0, 0) }
};
#endif /* UTIL_FORMAT_H */
