#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
# Override BOARD_ROOT to always point at this SDK's boards/ directory.
# Using CACHE FORCE ensures this wins over any -DBOARD_ROOT passed by the IDE
# or build system, preventing "board defined multiple times" errors when
# multiple SDK versions are present in the environment.
set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "" FORCE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(pwm)

target_sources(app PRIVATE src/main.c)
