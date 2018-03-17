# Copyright (c) 2017-2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)
project(avoutput-adaptation-layer-rpi)
include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC7)
webos_component(1 0 0)

webos_add_compiler_flags(ALL -std=c++11 -Wall -Wextra -fno-permissive)
#disable specific warnings
webos_add_compiler_flags(ALL -Wno-unused-parameter -Wno-type-limits -Wno-comment)
#promote specific warnings to errors
webos_add_compiler_flags(ALL -Werror=return-type  -Werror=reorder -Werror=uninitialized)
# Require that all undefined symbols are satisfied by the libraries from target_link_libraries()
webos_add_linker_options(ALL --no-undefined)
webos_add_linker_options(ALL --hash-style=gnu --as-needed --as-needed)

include(FindPkgConfig)

pkg_check_modules(AVALAPI REQUIRED avoutput-adaptation-layer-api)
include_directories(${AVALAPI_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${AVALAPI})

pkg_check_modules(PMLOG REQUIRED PmLogLib)
include_directories(${PMLOG_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS_OTHER})


pkg_check_modules(LIBDRM REQUIRED libdrm)
include_directories(${LIBDRM_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LIBDRM})

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS})

pkg_check_modules(UDEV REQUIRED libudev)
include_directories(${UDEV_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${UDEV_CFLAGS_OTHER})

include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/aval)
file(GLOB SOURCES src/*.cpp src/aval/*.cpp)


file(MAKE_DIRECTORY "include/public")

#add local include directories, accessible only by double quotes include.
webos_add_compiler_flags(ALL -iquote ${CMAKE_CURRENT_BINARY_DIR}/Configured/files/conf)
webos_add_compiler_flags(ALL -iquote ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-dynamic")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")

set(CMAKE_SHARED_MODULE_PREFIX "")
set(NO_SONAME)

add_library(aval-rpi SHARED ${SOURCES})
target_link_libraries(aval-rpi
        ${PMLOG_LDFLAGS}
        ${PBNJSON_CPP_LDFLAGS}
        ${GLIB2_LDFLAGS}
        ${UDEV_LDFLAGS}
        asound
        drm)

webos_build_library(TARGET aval-rpi)

install(TARGETS aval-rpi DESTINATION ${WEBOS_INSTALL_LIBDIR})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/files/conf/device-cap.json
        DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/aval
        )

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/files/conf/asound.conf DESTINATION ${WEBOS_INSTALL_SYSCONFDIR})

webos_configure_source_files(SOURCES files/conf/config.h)

webos_build_pkgconfig(files/pkgconfig/aval-impl)

add_executable(drmTest  tests/main.cpp tests/pattern.cpp)
target_link_libraries(drmTest drm aval-rpi)

set(WEBOS_CONFIG_BUILD_TESTS FALSE CACHE BOOL "Set to TRUE to enable tests compilation")
if (WEBOS_CONFIG_BUILD_TESTS)
    install(TARGETS drmTest
        DESTINATION ${WEBOS_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME}/test
        )

endif()
