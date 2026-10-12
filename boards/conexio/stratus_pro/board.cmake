# Copyright (c) 2026 Conexio Technologies, Inc
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_CONEXIO_STRATUS_PRO_NRF9161_NS OR CONFIG_BOARD_CONEXIO_STRATUS_PRO_NRF9151_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

if(CONFIG_BOARD_CONEXIO_STRATUS_PRO_NRF9161_NS)
  board_runner_args(jlink "--device=nRF9161_xxCA" "--speed=4000")
endif()

if(CONFIG_BOARD_CONEXIO_STRATUS_PRO_NRF9151_NS)
  board_runner_args(jlink "--device=nRF9151_xxCA" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
