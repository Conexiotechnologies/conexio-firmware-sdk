# NCS 3.4.1 Partition Porting — Per-Sample Change Summary

**Date applied:** 2025  
**SDK:** `/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk`  
**Reference sample (not modified):** `samples/conexio_stratus/at_client/`

All paths are relative to `samples/conexio_stratus/`.

---

## Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Change applied |
| ➖ | Not applicable (Category B — no nRF91 NS overlay) |
| ⚠ | Special case (see notes) |

---

## Category A — nRF91xx Full Treatment (27 samples)

### accelerometer
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ `set(BOARD_ROOT "../../.." CACHE PATH "" FORCE)` added before `find_package` |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already had `SB_CONFIG_BOOTLOADER_MCUBOOT=y` |

### adc_read
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### bme280
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### bme680
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### digital_read_write
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### digital_read
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### digital_write
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### dps310
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### ext_flash
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### i2c_read_write
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### low_power_mode
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### multi_sensor
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### npm_leds
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### npm1300_fuel_gauge
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### nvs_storage ⚠ (conflict resolved)
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes added; conflicting `storage_partition: partition@fa000` block removed (conflicts with `nrf91_bootloader_partitions.dtsi` which defines its own `storage_partition`). Only `/ { chosen { zephyr,nvs-flash = &flash0; }; };` retained. |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### rtc_alarm
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### rtos_threads
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### scd4x
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### sht3xd
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### sht4x
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### sht4x_power_gate
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### spi_read_write
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### timers
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### udp
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### watchdog
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

### wifi_scan ⚠ (sysbuild.conf special case)
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `boards/conexio_stratus_pro_nrf9161_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | ✅ `SB_CONFIG_BOOTLOADER_MCUBOOT=y` appended (existing `SB_CONFIG_WIFI_NRF700X` lines preserved) |

### work_queue
| File | Change |
|------|--------|
| `CMakeLists.txt` | ✅ BOARD_ROOT added |
| `boards/conexio_stratus_pro_nrf9151_ns.overlay` | ✅ Partition includes prepended |
| `prj.conf` | ✅ `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` appended |
| `sysbuild.conf` | — already correct |

---

## Category B — CMakeLists.txt Only (13 samples)

No nRF91xx NS overlay exists. Only BOARD_ROOT fix applied.

| Sample | CMakeLists.txt | Overlay | prj.conf | sysbuild.conf |
|--------|:-:|:-:|:-:|:-:|
| battery_adc | ✅ | ➖ | ➖ | ➖ |
| button | ✅ | ➖ | ➖ | ➖ |
| cellular_modem_battery_monitor | ✅ | ➖ | ➖ | ➖ |
| device_info | ✅ | ➖ | ➖ | ➖ |
| empty_template | ✅ | ➖ | ➖ | ➖ |
| full_modem_firmware_update | ✅ | ➖ | ➖ | ➖ |
| gnss | ✅ | ➖ | ➖ | ➖ |
| led_blink | ✅ | ➖ | ➖ | ➖ |
| logging_levels | ✅ | ➖ | ➖ | ➖ |
| mqtt_control | ✅ | ➖ | ➖ | ➖ |
| pwm | ✅ | ➖ | ➖ | ➖ |
| shell_commands | ✅ | ➖ | ➖ | ➖ |
| usefulsensors_person_sensor | ✅ | ➖ | ➖ | ➖ |

---

## Totals

| Change | Count |
|--------|-------|
| CMakeLists.txt — BOARD_ROOT added | 40 |
| Overlay files — partition includes prepended | 47 |
| prj.conf — `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` added | 27 |
| sysbuild.conf — `SB_CONFIG_BOOTLOADER_MCUBOOT=y` added | 1 (wifi_scan) |

---

## Build Verification

Representative sample built: **accelerometer** (`conexio_stratus_pro/nrf9151/ns`)

**Result: BUILD SUCCEEDED**
- MCUBoot built and signed
- Application: FLASH 43920 B / 417642 B, RAM 6008 B / 211608 B
- `dfu_application.zip` generated

The "board defined multiple times" error and the `nrf91_no_bootloader_partitions.dtsi`
`tfm_ps_partition` parse error are both resolved by these changes.
