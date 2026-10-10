# NCS 3.4.1 Partition Porting — Implementation Plan

**Reference sample (DO NOT TOUCH):**
`/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/at_client/`

**SDK root:** `/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk`

---

## Key Facts Discovered During Investigation

### Depth / BOARD_ROOT path
All 39 remaining samples live at exactly:
```
samples/conexio_stratus/<sample_name>/CMakeLists.txt
```
That is **3 levels deep** from the SDK root, so every sample uses the same BOARD_ROOT string:
```cmake
set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "" FORCE)
```
Place this line **immediately before** `find_package(Zephyr ...)`.

### sysbuild.conf — already correct except one sample
38 of 39 samples already have `SB_CONFIG_BOOTLOADER_MCUBOOT=y`.  
**Exception:** `wifi_scan` — its `sysbuild.conf` currently contains only WiFi-specific settings
(`SB_CONFIG_WIFI_NRF700X=y` and `SB_CONFIG_WIFI_NRF700X_SCAN_ONLY=y`). The line
`SB_CONFIG_BOOTLOADER_MCUBOOT=y` must be **appended** (not the file replaced).

### prj.conf — missing CONFIG_TFM_PROFILE_TYPE_MINIMAL=y everywhere
Zero out of 39 samples have it. Every nRF91 full-treatment sample needs it added.

### NCS partition dtsi files — confirmed present
```
/opt/nordic/ncs/v3.4.1/nrf/dts/samples/cellular/nrf91_bootloader_partitions.dtsi  ✓
/opt/nordic/ncs/v3.4.1/nrf/dts/samples/cellular/nrf91_sram_partitions.dtsi        ✓
```
These are reached via angle-bracket includes because the NCS `nrf` module registers
`nrf/dts` as a DTS search-path root in `nrf/zephyr/module.yml`.

### Overlay edit rule
Insert the two `#include` lines **at the top of each overlay file**, right after the
existing copyright/SPDX comment block and before any DTS node content.
```c
#include <samples/cellular/nrf91_bootloader_partitions.dtsi>
#include <samples/cellular/nrf91_sram_partitions.dtsi>
```
Do **not** remove any existing peripheral configuration in the overlay.

---

## Classification

### Category A — nRF91xx Full Treatment (27 samples)
Have at least one `*_nrf9151_ns.overlay`, `*_nrf9161_ns.overlay`, or `conexio_stratus_ns.overlay`.
All four changes apply: CMakeLists.txt, overlay file(s), prj.conf, and sysbuild.conf
(sysbuild only needs editing for `wifi_scan`; all others already correct).

| Sample | nRF9151_ns overlay | nRF9161_ns overlay | conexio_stratus_ns overlay | sysbuild.conf status |
|--------|:-:|:-:|:-:|---|
| accelerometer | ✓ | ✓ | — | MCUBOOT=y ✓ |
| adc_read | ✓ | — | — | MCUBOOT=y ✓ |
| bme280 | ✓ | ✓ | ✓ | MCUBOOT=y ✓ |
| bme680 | ✓ | ✓ | ✓ | MCUBOOT=y ✓ |
| digital_read_write | ✓ | — | — | MCUBOOT=y ✓ |
| digital_read | ✓ | ✓ | — | MCUBOOT=y ✓ |
| digital_write | ✓ | ✓ | — | MCUBOOT=y ✓ |
| dps310 | ✓ | ✓ | ✓ | MCUBOOT=y ✓ |
| ext_flash | ✓ | — | — | MCUBOOT=y ✓ |
| i2c_read_write | ✓ | — | — | MCUBOOT=y ✓ |
| low_power_mode | ✓ | ✓ | ✓ | MCUBOOT=y ✓ |
| multi_sensor | ✓ | ✓ | — | MCUBOOT=y ✓ |
| npm_leds | ✓ | — | — | MCUBOOT=y ✓ |
| npm1300_fuel_gauge | ✓ | ✓ | — | MCUBOOT=y ✓ |
| nvs_storage | ✓ | — | — | MCUBOOT=y ✓ (see ⚠ below) |
| rtc_alarm | ✓ | — | — | MCUBOOT=y ✓ |
| rtos_threads | ✓ | — | — | MCUBOOT=y ✓ |
| scd4x | ✓ | — | — | MCUBOOT=y ✓ |
| sht3xd | ✓ | ✓ | ✓ | MCUBOOT=y ✓ |
| sht4x_power_gate | ✓ | ✓ | — | MCUBOOT=y ✓ |
| sht4x | ✓ | ✓ | — | MCUBOOT=y ✓ |
| spi_read_write | ✓ | — | — | MCUBOOT=y ✓ |
| timers | ✓ | — | — | MCUBOOT=y ✓ |
| udp | ✓ | ✓ | ✓ | MCUBOOT=y ✓ |
| watchdog | ✓ | — | — | MCUBOOT=y ✓ |
| wifi_scan | ✓ | ✓ | — | ❌ needs MCUBOOT=y added |
| work_queue | ✓ | — | — | MCUBOOT=y ✓ |

### Category B — CMakeLists.txt Only (12 samples)
No nRF91xx NS overlay exists → conservative rule: BOARD_ROOT fix only.
Do NOT create overlay files, do NOT modify sysbuild.conf, do NOT modify prj.conf.

| Sample | Reason |
|--------|--------|
| battery_adc | no boards/ dir |
| button | no boards/ dir |
| cellular_modem_battery_monitor | no boards/ dir (nRF91 modem user, but no NS overlay) |
| device_info | no boards/ dir (nRF91 modem user, but no NS overlay) |
| empty_template | no boards/ dir |
| full_modem_firmware_update | no boards/ dir (nRF91 modem firmware, but no NS overlay) |
| gnss | no boards/ dir (nRF91 GNSS, but no NS overlay) |
| led_blink | no boards/ dir |
| logging_levels | no boards/ dir |
| mqtt_control | no boards/ dir (nRF91 cellular, but no NS overlay) |
| pwm | no boards/ dir |
| shell_commands | no boards/ dir |
| usefulsensors_person_sensor | no boards/ dir |

---

## Special Cases / Conflicts

### ⚠ nvs_storage — overlay conflict with storage_partition
`boards/conexio_stratus_pro_nrf9151_ns.overlay` currently redefines:
```c
&flash0 {
    partitions {
        storage_partition: partition@fa000 {
            label = "storage";
            reg = <0x000fa000 0x00006000>;
        };
    };
};
```
`nrf91_bootloader_partitions.dtsi` already defines `storage_partition: partition@f0000`
(64 KB, addresses 0xf0000–0xfffff). The overlay's `@fa000` node falls **within** that range,
and the label `storage_partition` would be declared on two different nodes — a DTS error.

**Required additional edit for nvs_storage only:** After inserting the partition includes,
**remove** the `&flash0 { partitions { storage_partition: partition@fa000 { ... }; }; };`
block from the overlay. Keep only the `/ { chosen { zephyr,nvs-flash = &flash0; }; };` part.
The `storage_partition` provided by `nrf91_bootloader_partitions.dtsi` (64 KB) is suitable for NVS.

Final nvs_storage overlay should look like:
```c
/*
 * Copyright (c) 2025 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <samples/cellular/nrf91_bootloader_partitions.dtsi>
#include <samples/cellular/nrf91_sram_partitions.dtsi>

/ {
	chosen {
		zephyr,nvs-flash = &flash0;
	};
};
```

### ⚠ wifi_scan — sysbuild.conf must preserve WiFi settings
`sysbuild.conf` currently:
```
SB_CONFIG_WIFI_NRF700X=y
SB_CONFIG_WIFI_NRF700X_SCAN_ONLY=y
```
Append — do **not** replace:
```
SB_CONFIG_BOOTLOADER_MCUBOOT=y
```
Final file has all three lines.

---

## Implementation Steps (ordered by dependency)

### Step 1 — CMakeLists.txt BOARD_ROOT fix (all 39 samples)

For **every** sample listed in Category A and Category B, insert this line immediately
before `find_package(Zephyr ...)`:
```cmake
set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "" FORCE)
```

If the existing `find_package(Zephyr HINTS ...)` line uses `HINTS` (without `REQUIRED`),
leave it as-is; only prepend the `set(BOARD_ROOT ...)` line. Do not add REQUIRED.

Files (39 total, all under `/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/`):
```
accelerometer/CMakeLists.txt
adc_read/CMakeLists.txt
battery_adc/CMakeLists.txt
bme280/CMakeLists.txt
bme680/CMakeLists.txt
button/CMakeLists.txt
cellular_modem_battery_monitor/CMakeLists.txt
device_info/CMakeLists.txt
digital_read_write/CMakeLists.txt
digital_read/CMakeLists.txt
digital_write/CMakeLists.txt
dps310/CMakeLists.txt
empty_template/CMakeLists.txt
ext_flash/CMakeLists.txt
full_modem_firmware_update/CMakeLists.txt
gnss/CMakeLists.txt
i2c_read_write/CMakeLists.txt
led_blink/CMakeLists.txt
logging_levels/CMakeLists.txt
low_power_mode/CMakeLists.txt
mqtt_control/CMakeLists.txt
multi_sensor/CMakeLists.txt
npm_leds/CMakeLists.txt
npm1300_fuel_gauge/CMakeLists.txt
nvs_storage/CMakeLists.txt
pwm/CMakeLists.txt
rtc_alarm/CMakeLists.txt
rtos_threads/CMakeLists.txt
scd4x/CMakeLists.txt
shell_commands/CMakeLists.txt
sht3xd/CMakeLists.txt
sht4x_power_gate/CMakeLists.txt
sht4x/CMakeLists.txt
spi_read_write/CMakeLists.txt
timers/CMakeLists.txt
udp/CMakeLists.txt
usefulsensors_person_sensor/CMakeLists.txt
watchdog/CMakeLists.txt
wifi_scan/CMakeLists.txt
work_queue/CMakeLists.txt
```

Verify: `grep -r "set(BOARD_ROOT" /opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/ | grep -v at_client | wc -l` should return 39.

---

### Step 2 — Overlay partition includes (Category A, 27 samples)

For each overlay file listed below, **prepend** the two include lines right after the
existing copyright/SPDX comment block (before the first DTS node line):
```c
#include <samples/cellular/nrf91_bootloader_partitions.dtsi>
#include <samples/cellular/nrf91_sram_partitions.dtsi>
```

**Samples with a single nrf9151_ns overlay — edit 1 file each:**
```
adc_read/boards/conexio_stratus_pro_nrf9151_ns.overlay
digital_read_write/boards/conexio_stratus_pro_nrf9151_ns.overlay
ext_flash/boards/conexio_stratus_pro_nrf9151_ns.overlay
i2c_read_write/boards/conexio_stratus_pro_nrf9151_ns.overlay
npm_leds/boards/conexio_stratus_pro_nrf9151_ns.overlay
nvs_storage/boards/conexio_stratus_pro_nrf9151_ns.overlay    ← also remove storage_partition redefinition (see conflict note above)
rtc_alarm/boards/conexio_stratus_pro_nrf9151_ns.overlay
rtos_threads/boards/conexio_stratus_pro_nrf9151_ns.overlay
scd4x/boards/conexio_stratus_pro_nrf9151_ns.overlay
spi_read_write/boards/conexio_stratus_pro_nrf9151_ns.overlay
timers/boards/conexio_stratus_pro_nrf9151_ns.overlay
watchdog/boards/conexio_stratus_pro_nrf9151_ns.overlay
work_queue/boards/conexio_stratus_pro_nrf9151_ns.overlay
```

**Samples with nrf9151_ns + nrf9161_ns overlays — edit 2 files each:**
```
accelerometer/boards/conexio_stratus_pro_nrf9151_ns.overlay
accelerometer/boards/conexio_stratus_pro_nrf9161_ns.overlay

digital_read/boards/conexio_stratus_pro_nrf9151_ns.overlay
digital_read/boards/conexio_stratus_pro_nrf9161_ns.overlay

digital_write/boards/conexio_stratus_pro_nrf9151_ns.overlay
digital_write/boards/conexio_stratus_pro_nrf9161_ns.overlay

multi_sensor/boards/conexio_stratus_pro_nrf9151_ns.overlay
multi_sensor/boards/conexio_stratus_pro_nrf9161_ns.overlay

npm1300_fuel_gauge/boards/conexio_stratus_pro_nrf9151_ns.overlay
npm1300_fuel_gauge/boards/conexio_stratus_pro_nrf9161_ns.overlay

sht4x_power_gate/boards/conexio_stratus_pro_nrf9151_ns.overlay
sht4x_power_gate/boards/conexio_stratus_pro_nrf9161_ns.overlay

sht4x/boards/conexio_stratus_pro_nrf9151_ns.overlay
sht4x/boards/conexio_stratus_pro_nrf9161_ns.overlay

wifi_scan/boards/conexio_stratus_pro_nrf9151_ns.overlay
wifi_scan/boards/conexio_stratus_pro_nrf9161_ns.overlay
```

**Samples with nrf9151_ns + nrf9161_ns + conexio_stratus_ns overlays — edit 3 files each:**
```
bme280/boards/conexio_stratus_pro_nrf9151_ns.overlay
bme280/boards/conexio_stratus_pro_nrf9161_ns.overlay
bme280/boards/conexio_stratus_ns.overlay

bme680/boards/conexio_stratus_pro_nrf9151_ns.overlay
bme680/boards/conexio_stratus_pro_nrf9161_ns.overlay
bme680/boards/conexio_stratus_ns.overlay

dps310/boards/conexio_stratus_pro_nrf9151_ns.overlay
dps310/boards/conexio_stratus_pro_nrf9161_ns.overlay
dps310/boards/conexio_stratus_ns.overlay

low_power_mode/boards/conexio_stratus_pro_nrf9151_ns.overlay
low_power_mode/boards/conexio_stratus_pro_nrf9161_ns.overlay
low_power_mode/boards/conexio_stratus_ns.overlay

sht3xd/boards/conexio_stratus_pro_nrf9151_ns.overlay
sht3xd/boards/conexio_stratus_pro_nrf9161_ns.overlay
sht3xd/boards/conexio_stratus_ns.overlay

udp/boards/conexio_stratus_pro_nrf9151_ns.overlay
udp/boards/conexio_stratus_pro_nrf9161_ns.overlay
udp/boards/conexio_stratus_ns.overlay
```

Total overlay file edits: 13 + 16 + 18 = **47 files** across 27 samples.

Verify: Each edited overlay should have `#include <samples/cellular/nrf91_bootloader_partitions.dtsi>` as one of its first non-comment lines.

---

### Step 3 — prj.conf CONFIG_TFM_PROFILE_TYPE_MINIMAL (Category A, 27 samples)

Add the following line to the end of each sample's `prj.conf` (all 27 Category A samples):
```
# TF-M minimal profile (required when not using full TF-M PS/ITS/OTP storage)
CONFIG_TFM_PROFILE_TYPE_MINIMAL=y
```

Files (all under `/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/`):
```
accelerometer/prj.conf
adc_read/prj.conf
bme280/prj.conf
bme680/prj.conf
digital_read_write/prj.conf
digital_read/prj.conf
digital_write/prj.conf
dps310/prj.conf
ext_flash/prj.conf
i2c_read_write/prj.conf
low_power_mode/prj.conf
multi_sensor/prj.conf
npm_leds/prj.conf
npm1300_fuel_gauge/prj.conf
nvs_storage/prj.conf
rtc_alarm/prj.conf
rtos_threads/prj.conf
scd4x/prj.conf
sht3xd/prj.conf
sht4x_power_gate/prj.conf
sht4x/prj.conf
spi_read_write/prj.conf
timers/prj.conf
udp/prj.conf
watchdog/prj.conf
wifi_scan/prj.conf
work_queue/prj.conf
```

Verify: `grep -r "CONFIG_TFM_PROFILE_TYPE_MINIMAL=y" /opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/ | grep -v at_client | wc -l` should return 27.

---

### Step 4 — wifi_scan sysbuild.conf: add MCUBOOT line

Append `SB_CONFIG_BOOTLOADER_MCUBOOT=y` to:
```
/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/wifi_scan/sysbuild.conf
```

Final content of `wifi_scan/sysbuild.conf`:
```conf
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
SB_CONFIG_WIFI_NRF700X=y
SB_CONFIG_WIFI_NRF700X_SCAN_ONLY=y
SB_CONFIG_BOOTLOADER_MCUBOOT=y
```

Verify: `grep "SB_CONFIG_BOOTLOADER_MCUBOOT" .../wifi_scan/sysbuild.conf` returns `SB_CONFIG_BOOTLOADER_MCUBOOT=y`.

---

## Per-Sample Quick Reference

All paths relative to `/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/`.

### accelerometer
- **CMakeLists.txt**: add `set(BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "" FORCE)` before `find_package`
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 LIS2DH sensor node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content as nrf9151 overlay)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: already has `SB_CONFIG_BOOTLOADER_MCUBOOT=y` ✓

### adc_read
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: ADC channel@0 + zephyr,user node)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### battery_adc *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir — skip overlay/sysbuild/prj.conf changes

### bme280
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 bme280@76 node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content)
- **boards/conexio_stratus_ns.overlay**: prepend partition includes (same content — nRF9160 Stratus board)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### bme680
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 bme688@76 node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes
- **boards/conexio_stratus_ns.overlay**: prepend partition includes
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### button *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### cellular_modem_battery_monitor *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir (uses nRF91 modem but no NS overlay)

### device_info *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### digital_read_write
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: gpio_inputs + gpio_outputs nodes)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### digital_read
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: pininput gpio alias + gpio_keys node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### digital_write
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: gpiopin alias + gpio_keys node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### dps310
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 DPS310@77 node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes
- **boards/conexio_stratus_ns.overlay**: prepend partition includes
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### empty_template *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### ext_flash
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 eeprom@50 node)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### full_modem_firmware_update *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir (nRF91 modem firmware update but no NS overlay)

### gnss *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### i2c_read_write
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 lis2dh@18 + eeprom@50 nodes)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### led_blink *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### logging_levels *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### low_power_mode
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: uart0 delete-property node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes
- **boards/conexio_stratus_ns.overlay**: prepend partition includes
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### mqtt_control *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir (nRF91 cellular but no NS overlay)

### multi_sensor
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 sht4x@44 node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### npm_leds
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2_default pull-up + pmic_main leds node)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### npm1300_fuel_gauge
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2_default pull-up group)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### nvs_storage ⚠ (conflict — see Special Cases above)
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes AND remove the
  `&flash0 { partitions { storage_partition: partition@fa000 { ... }; }; };` block (leave only
  the `/ { chosen { zephyr,nvs-flash = &flash0; }; };` section)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### pwm *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### rtc_alarm
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: rtc0 enable node)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### rtos_threads
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: led0/sw0 aliases)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### scd4x
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 scd4x@62 node)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### shell_commands *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### sht3xd
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 sht3xd@44 node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes
- **boards/conexio_stratus_ns.overlay**: prepend partition includes
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### sht4x_power_gate
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 sht4x@44 + gpio power alias)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### sht4x
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: i2c2 sht4x@44 node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### spi_read_write
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: spi3 device@0 node)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### timers
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: led0/sw0 aliases)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### udp
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: uart0 delete-property)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes
- **boards/conexio_stratus_ns.overlay**: prepend partition includes
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### usefulsensors_person_sensor *(Category B — CMakeLists.txt only)*
- **CMakeLists.txt**: add BOARD_ROOT
- No boards/ dir

### watchdog
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: explanatory comment block, no DTS nodes)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

### wifi_scan ⚠ (sysbuild.conf special case)
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: edge_connector_spi nrf7002@0 node)
- **boards/conexio_stratus_pro_nrf9161_ns.overlay**: prepend partition includes (same content)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: append `SB_CONFIG_BOOTLOADER_MCUBOOT=y` (keep existing WiFi lines)

### work_queue
- **CMakeLists.txt**: add BOARD_ROOT
- **boards/conexio_stratus_pro_nrf9151_ns.overlay**: prepend partition includes (currently: led0/sw0 aliases)
- **prj.conf**: append `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`
- **sysbuild.conf**: ✓

---

## Summary Totals

| Change | Count |
|--------|-------|
| CMakeLists.txt files edited (BOARD_ROOT) | 40 (plan said 39; actual sample count excluding at_client is 40) |
| Overlay files edited (partition includes prepended) | 47 |
| prj.conf files edited (TFM_MINIMAL added) | 27 |
| sysbuild.conf files edited (MCUBOOT added) | 1 (wifi_scan only) |
| Files requiring creation | 0 |
| Samples to skip entirely | 1 (at_client) |

---

## Verification (appended after implementation)

### Structural verification (grep counts)

| Check | Expected | Actual |
|-------|----------|--------|
| `set(BOARD_ROOT` in samples/ (excl. at_client) | 39 (plan) / 40 (actual) | **40** ✓ |
| `nrf91_bootloader_partitions` in overlays (excl. at_client) | 47 | **47** ✓ |
| `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` in samples/ (excl. at_client) | 27 | **27** ✓ |
| `SB_CONFIG_BOOTLOADER_MCUBOOT=y` in wifi_scan/sysbuild.conf | present | **present** ✓ |
| NCS dtsi `nrf91_bootloader_partitions.dtsi` exists | yes | **yes** ✓ |
| NCS dtsi `nrf91_sram_partitions.dtsi` exists | yes | **yes** ✓ |

### BOARD_ROOT line placement
Verified in `accelerometer/CMakeLists.txt` — `set(BOARD_ROOT ...)` appears **immediately before**
`find_package(Zephyr ...)`, with the correct `../../..` depth for samples/conexio_stratus/<name>/.

### Overlay angle-bracket includes
Verified in `accelerometer/boards/conexio_stratus_pro_nrf9151_ns.overlay` — includes appear
after the copyright+SPDX comment block, before the first DTS node.

### nvs_storage special case
Verified: `&flash0 { partitions { storage_partition: partition@fa000 ... } }` block removed;
only `/ { chosen { zephyr,nvs-flash = &flash0; }; };` retained, plus the two partition includes.

### wifi_scan sysbuild.conf
Verified: existing `SB_CONFIG_WIFI_NRF700X=y` and `SB_CONFIG_WIFI_NRF700X_SCAN_ONLY=y` lines
preserved; `SB_CONFIG_BOOTLOADER_MCUBOOT=y` appended.

### West build — accelerometer sample (representative nRF91 build)

Command:
```
cd /opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/accelerometer
ZEPHYR_SDK_INSTALL_DIR=/opt/nordic/ncs/toolchains/97226bad62/opt/zephyr-sdk
ZEPHYR_BASE=/opt/nordic/ncs/v3.4.1/zephyr
west build --build-dir /tmp/test_accel_build . --pristine \
  --board conexio_stratus_pro/nrf9151/ns \
  -- -DNCS_TOOLCHAIN_VERSION=NONE -DBOARD_ROOT=/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk
```

**Result: BUILD SUCCEEDED**

Key output:
- Board: `conexio_stratus_pro, qualifiers: nrf9151/ns` — found (no "defined multiple times" error)
- MCUBoot built and signed
- Application built: FLASH 43920 B / 417642 B (10.52%), RAM 6008 B / 211608 B (2.84%)
- `dfu_application.zip` generated

Note: The build requires `ZEPHYR_SDK_INSTALL_DIR` to be set pointing to the NCS toolchain's
zephyr-sdk directory. The IDE (nRF Connect) sets this automatically; bare `west build` calls
need it exported.
