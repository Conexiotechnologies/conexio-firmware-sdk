#
# Copyright (c) 2024 Conexio Technologies, Inc
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(datacake)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

zephyr_include_directories(src)

# Include application events and configuration headers
zephyr_library_include_directories(
  src/env_sensors
)

# Application sources
add_subdirectory_ifdef(CONFIG_ENVIRONMENT_SENSORS src/env_sensors)
add_subdirectory_ifdef(CONFIG_NRF_FUEL_GAUGE src/fuel_gauge)
add_subdirectory_ifdef(CONFIG_MOTION_SENSOR src/motion_sensor)
