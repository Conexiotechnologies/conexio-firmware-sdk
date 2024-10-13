#
# Copyright (c) 2024 Conexio Technologies, Inc.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/motion_sensor.c)