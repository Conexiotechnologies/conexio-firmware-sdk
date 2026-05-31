# Copyright (c) 2025 Conexio Technologies, Inc
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(i2c_read_write)

target_sources(app PRIVATE src/main.c)
