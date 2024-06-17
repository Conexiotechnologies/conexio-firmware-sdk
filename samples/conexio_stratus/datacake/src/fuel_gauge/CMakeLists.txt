# Copyright (c) 2024 Conexio Technologies, Inc

zephyr_include_directories(.)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fuel_gauge.c)
