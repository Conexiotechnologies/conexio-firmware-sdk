#
# Copyright (c) 2019 Nordic Semiconductor
# Copyright (c) 2022 Conexio Technologies, Inc.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/env_sensors.c)
