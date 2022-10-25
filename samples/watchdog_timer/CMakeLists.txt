#
# Copyright (c) 2018 Nordic Semiconductor ASA
# Copyright (c) 2022 Conexio Technologies, Inc
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(watchdog)

target_sources(app PRIVATE src/main.c)

zephyr_include_directories(src)

# Include application events and configuration headers
zephyr_library_include_directories(src/watchdog)

# Application sources
add_subdirectory_ifdef(CONFIG_WATCHDOG src/watchdog)

