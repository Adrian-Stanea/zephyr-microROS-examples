# Copyright (c) 2025 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(LIBMICROROS_DIR $ENV{ZEPHYR_BASE}/../modules/lib/micro_ros_zephyr_module/modules/libmicroros/)
list(APPEND ZEPHYR_EXTRA_MODULES "${LIBMICROROS_DIR}")
message(STATUS "LIBMICROROS_DIR: ${LIBMICROROS_DIR}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app LANGUAGES C)

# Include compiler flags to enable source navigation with ELF File Explorer
zephyr_cc_option(-fstack-usage)
zephyr_cc_option(-fdump-ipa-cgraph)
zephyr_cc_option(-gdwarf-4)

target_sources(app PRIVATE src/main.c)
