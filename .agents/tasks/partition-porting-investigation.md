# NCS 3.2.1 → 3.4.1 Partition Porting Investigation
## Conexio Stratus Pro — DT Partition Setup

**Investigated:** October 2025  
**Board:** `conexio_stratus_pro/nrf9151/ns`  
**Sample:** `samples/conexio_stratus/at_client`

---

## Summary Answer

There are **four distinct problems** with the current port. The two most critical will cause a DTS compilation error due to overlapping flash partitions. The other two cause a logical mismatch between the build configuration and the partition layout at flash time.

| # | Severity | File | Problem |
|---|----------|------|---------|
| 1 | **BLOCKING** | `dts/nrf91_no_bootloader_partitions.dtsi` | Missing 3 `/delete-node/` directives for TF-M partitions |
| 2 | **BLOCKING** | `dts/nrf91_bootloader_partitions.dtsi` | Missing 1 `/delete-node/` directive for `tfm_ps_partition` |
| 3 | **IMPORTANT** | `at_client/boards/conexio_stratus_pro_nrf9151_ns.overlay` | Absolute hardcoded include paths; should use NCS angle-bracket paths |
| 4 | **IMPORTANT** | `at_client/sysbuild.conf` + overlay | MCUboot enabled in sysbuild but overlay uses the no-bootloader partition layout |

---

## Background: What Changed Between 3.2.1 and 3.4.1

### NCS 3.2.1 Approach
In 3.2.1, the Conexio board defined its partitions entirely at the board level:
- `conexio_stratus_pro_common.dtsi` contained an inline `&flash0 { partitions { ... } }` block using the old `fixed-partitions` format
- A separate `conexio_stratus_pro_partition_conf.dtsi` adjusted partition sizes
- Samples required **no overlay** for partitions (the at_client had no `boards/` overlay directory)

### NCS 3.4.1 Approach
In 3.4.1, Zephyr introduced `zephyr,mapped-partition` and the NCS Partition Manager was replaced by DTS-based partitions:
- A new canonical file `zephyr/dts/vendor/nordic/nrf91xx_partition.dtsi` defines the **full MCUboot + TF-M partition layout** as the board default
- The board's `_common.dtsi` includes this via `#include <nordic/nrf91xx_partition.dtsi>` (same as Nordic's own nrf9151dk)
- Each **sample** must now provide a board-specific overlay that deletes the default nodes and redefines them to match the actual build configuration (with or without MCUboot, with or without full TF-M)
- NCS ships ready-to-use overlay dtsi files in `nrf/dts/samples/cellular/`

The developer correctly identified that overlays are needed and created the `boards/` directory and overlay file. The `conexio_stratus_pro_common.dtsi` was also correctly updated to drop the old inline partition block and add `#include <nordic/nrf91xx_partition.dtsi>` at the bottom. The approach is right; the execution has four gaps.

---

## Issue 1 (BLOCKING): Missing `/delete-node/` Directives in `nrf91_no_bootloader_partitions.dtsi`

### Evidence

**User's file:** `/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/dts/nrf91_no_bootloader_partitions.dtsi` (lines 9–12):
```c
/delete-node/ &boot_partition;
/delete-node/ &slot0_partition;
/delete-node/ &slot1_partition;
/delete-node/ &storage_partition;
```

**NCS reference file:** `/opt/nordic/ncs/v3.4.1/nrf/dts/samples/cellular/nrf91_no_bootloader_partitions.dtsi` (lines 9–15):
```c
/delete-node/ &boot_partition;
/delete-node/ &slot0_partition;
/delete-node/ &slot1_partition;
/delete-node/ &tfm_ps_partition;
/delete-node/ &tfm_its_partition;
/delete-node/ &tfm_otp_partition;
/delete-node/ &storage_partition;
```

**`diff` output confirms exactly:**
```
9a10,12
> /delete-node/ &tfm_ps_partition;
> /delete-node/ &tfm_its_partition;
> /delete-node/ &tfm_otp_partition;
```

### Why This Causes a Failure

`conexio_stratus_pro_common.dtsi` (last line) includes `<nordic/nrf91xx_partition.dtsi>` which defines **seven flash partition nodes**, including:

```
tfm_ps_partition: partition@f0000  { reg = <0x000f0000 0x00004000>; }  // Protected Storage
tfm_its_partition: partition@f4000 { reg = <0x000f4000 0x00002000>; }  // Internal Trusted Storage  
tfm_otp_partition: partition@f6000 { reg = <0x000f6000 0x00002000>; }  // OTP/NV counters
```

The no-bootloader layout redefines `slot0_ns_partition` to start at `0x00008000` with size `0xf0000`, meaning it occupies flash from `0x8000` to `0xf8000`. The three undestroyed TF-M nodes sit at addresses `0xf0000`, `0xf4000`, and `0xf6000` — all **within** the range of the new `slot0_ns_partition`. The DTS compiler will error with overlapping partition regions.

### Fix

Add the three missing lines to the user's `dts/nrf91_no_bootloader_partitions.dtsi` after line 11 (`/delete-node/ &slot1_partition;`):

```c
/delete-node/ &boot_partition;
/delete-node/ &slot0_partition;
/delete-node/ &slot1_partition;
/delete-node/ &tfm_ps_partition;    /* ADD THIS */
/delete-node/ &tfm_its_partition;   /* ADD THIS */
/delete-node/ &tfm_otp_partition;   /* ADD THIS */
/delete-node/ &storage_partition;
```

---

## Issue 2 (BLOCKING): Missing `/delete-node/` in `nrf91_bootloader_partitions.dtsi`

### Evidence

**User's file:** `/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/dts/nrf91_bootloader_partitions.dtsi` (lines 9–14):
```c
/delete-node/ &boot_partition;
/delete-node/ &slot0_partition;
/delete-node/ &slot1_partition;
/delete-node/ &tfm_its_partition;
/delete-node/ &tfm_otp_partition;
/delete-node/ &storage_partition;
```

**NCS reference:** `/opt/nordic/ncs/v3.4.1/nrf/dts/samples/cellular/nrf91_bootloader_partitions.dtsi` (lines 9–15):
```c
/delete-node/ &boot_partition;
/delete-node/ &slot0_partition;
/delete-node/ &slot1_partition;
/delete-node/ &tfm_ps_partition;    /* MISSING */
/delete-node/ &tfm_its_partition;
/delete-node/ &tfm_otp_partition;
/delete-node/ &storage_partition;
```

`diff` confirms: `9a10 > /delete-node/ &tfm_ps_partition;`

### Why This Causes a Failure

The bootloader layout redefines `storage_partition` at `0xf0000` (64 KB). `tfm_ps_partition` from `nrf91xx_partition.dtsi` is also at `0xf0000`. Without deleting `tfm_ps_partition` first, the DTS will have two nodes at the same address in `flash0`.

### Fix

Add the missing line to `dts/nrf91_bootloader_partitions.dtsi` after `/delete-node/ &slot1_partition;`:

```c
/delete-node/ &tfm_ps_partition;    /* ADD THIS */
```

---

## Issue 3 (IMPORTANT): Absolute Hardcoded Include Paths in Overlay

### Evidence

`/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/samples/conexio_stratus/at_client/boards/conexio_stratus_pro_nrf9151_ns.overlay`:
```c
#include "/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/dts/nrf91_no_bootloader_partitions.dtsi"
#include "/opt/nordic/ncs/v3.4.1/conexio-firmware-sdk/dts/nrf91_sram_partitions.dtsi"
```

**NCS reference overlay** (`nrf/samples/cellular/at_client/boards/nrf9151dk_nrf9151_ns.overlay`):
```c
#include <samples/cellular/nrf91_no_bootloader_partitions.dtsi>
#include <samples/cellular/nrf91_sram_partitions.dtsi>
```

### Why This Is a Problem

1. **The files included are not actually needed in the SDK** — the NCS `nrf` module registers `nrf/dts` as a DTS search-path root via `dts_root: .` in `/opt/nordic/ncs/v3.4.1/nrf/zephyr/module.yml`. The reference files already live at `nrf/dts/samples/cellular/` and are reachable via angle-bracket includes. The user's custom copies in `conexio-firmware-sdk/dts/` are redundant.

2. **Absolute paths are not portable** — they will break on any machine where NCS is installed at a different path, or if the SDK is relocated.

3. **The custom copies have bugs** (Issues 1 and 2 above). Using the NCS reference files directly eliminates both bugs simultaneously.

### Fix

Replace the overlay content with angle-bracket includes pointing directly to the NCS reference files:

```c
/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <samples/cellular/nrf91_no_bootloader_partitions.dtsi>

#include <samples/cellular/nrf91_sram_partitions.dtsi>
```

This is the exact pattern used by the official NCS at_client sample for nrf9151dk.

---

## Issue 4 (IMPORTANT): Inconsistency Between `sysbuild.conf` and Partition Layout

### Evidence

**`sysbuild.conf`** (both NCS 3.2.1 and 3.4.1 versions):
```
SB_CONFIG_BOOTLOADER_MCUBOOT=y
```

**Overlay** (using `nrf91_no_bootloader_partitions.dtsi`):
```
slot0_ns_partition starts at 0x00008000 (no space for MCUboot at flash start)
```

### Why This Is a Problem

`SB_CONFIG_BOOTLOADER_MCUBOOT=y` tells sysbuild to build MCUboot as a separate image and place it at the start of flash (`boot_partition` at address `0x0`, size 64 KB). But the `nrf91_no_bootloader_partitions.dtsi` layout allocates the **entire** flash from `0x0` to `0xf7fff` to the application (`slot0_partition`), with the secure image at `0x0` and the NS image starting at `0x8000`. 

MCUboot's 64 KB image occupies `0x0–0x10000` and would overwrite the first half of the secure application partition (`slot0_s_partition` at `0x0–0x7fff`).

The official NCS at_client sample does **not** use MCUboot — it has no `sysbuild.conf` at all and uses `nrf91_no_bootloader_partitions.dtsi` only.

### Fix (choose one)

**Option A — No MCUboot (matches the official NCS at_client pattern):**

Remove `SB_CONFIG_BOOTLOADER_MCUBOOT=y` from `sysbuild.conf` (or delete the file). Keep the overlay with `nrf91_no_bootloader_partitions.dtsi`.

Also add `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` to `prj.conf` to match the NCS at_client reference (see Issue 5 below).

**Option B — Keep MCUboot:**

Change the overlay to include `nrf91_bootloader_partitions.dtsi` instead:

```c
#include <samples/cellular/nrf91_bootloader_partitions.dtsi>
#include <samples/cellular/nrf91_sram_partitions.dtsi>
```

This provides a `boot_partition` at `0x0` (64 KB for MCUboot) and places `slot0_ns_partition` at `0x18000` with 416 KB for the NS image. The `zephyr,code-partition = &slot0_ns_partition` in `conexio_stratus_pro_nrf9151_ns.dts` is correct for both options.

---

## Issue 5 (ADVISORY): Missing `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` in `prj.conf`

### Evidence

**Conexio at_client `prj.conf`:** No `CONFIG_TFM_PROFILE_TYPE_MINIMAL` entry.

**Official NCS at_client `prj.conf`:**
```
CONFIG_TFM_PROFILE_TYPE_MINIMAL=y
```

### Why This Matters

Without `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`, the full TF-M profile is built. The full profile requires Protected Storage and Internal Trusted Storage partitions (`tfm_ps_partition`, `tfm_its_partition`, `tfm_otp_partition`) in flash. Both `nrf91_no_bootloader_partitions.dtsi` and `nrf91_bootloader_partitions.dtsi` delete those nodes and do not replace them — they are designed specifically for the **minimal** TF-M profile where only the secure image and NS application partitions are needed.

### Fix

Add to `samples/conexio_stratus/at_client/prj.conf`:

```
# TF-M minimal profile (required when not using full TF-M PS/ITS/OTP storage)
CONFIG_TFM_PROFILE_TYPE_MINIMAL=y
```

---

## What Is Correct

The following aspects of the port are **correctly implemented** and do not need changes:

1. **`conexio_stratus_pro_common.dtsi` drops the old partition block and includes `nrf91xx_partition.dtsi`** — The 3.2.1 board had inline `&flash0 { partitions { compatible = "fixed-partitions"; ... } }` and `reserved-memory` nodes. The 3.4.1 board correctly removes all of that and adds `#include <nordic/nrf91xx_partition.dtsi>` at the end, matching exactly what Nordic's own `nrf9151dk_nrf9151_common.dtsi` does.

2. **`conexio_stratus_pro_nrf9151_ns.dts` updated `zephyr,sram`** — Correctly changed from `&sram0_ns` (3.2.1) to `&sram0_ns_app` (3.4.1), because `nrf91xx_partition.dtsi` restructures SRAM to have `sram0_ns` as a container with `sram0_ns_modem` and `sram0_ns_app` subnodes. The application must point to `sram0_ns_app`, not the parent.

3. **`nrf91_sram_partitions.dtsi` is a perfect copy** — `diff` returns no differences between the user's copy and the NCS reference.

4. **Board structure files are correct** — `board.yml`, `board.cmake`, `Kconfig.defconfig`, `Kconfig.defconfig.nrf9151`, `Kconfig.conexio_stratus_pro`, `pre_dt_board.cmake`, and the `_ns.yaml` files all look correct for NCS 3.4.1.

5. **`conexio_stratus_pro_nrf9151_ns.dts` SoC include** — Correctly uses `#include <arm/nordic/nrf9151_ns_laca.dtsi>` matching the nrf9151dk reference board.

6. **`Kconfig.sysbuild`** — Matches the NCS reference; correctly sets `PARTITION_MANAGER default n` to disable the old Partition Manager.

7. **App overlay `boards/` directory** — Correct approach. The per-sample overlay is the right place to set partition layout, consistent with how Nordic structures its own samples.

---

## Recommended Action Order

1. **Fix the overlay** (Issue 3): Replace absolute paths with `<samples/cellular/...>` includes. This also sidesteps Issues 1 and 2 entirely by using the correct NCS files.

2. **Resolve the MCUboot vs no-bootloader decision** (Issue 4): Choose Option A (remove MCUboot) or Option B (use bootloader partitions overlay).

3. **Add `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y`** to `prj.conf` (Issue 5).

4. **If keeping the custom dtsi copies** in `conexio-firmware-sdk/dts/`: Fix both files (Issues 1 and 2) by adding the missing `/delete-node/` lines. But the simpler path is step 1 above — use the NCS reference files directly.

---

## Quick Fix (Minimal Change Path)

Replace the entire content of  
`samples/conexio_stratus/at_client/boards/conexio_stratus_pro_nrf9151_ns.overlay`  
with:

```c
/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <samples/cellular/nrf91_no_bootloader_partitions.dtsi>

#include <samples/cellular/nrf91_sram_partitions.dtsi>
```

Remove `SB_CONFIG_BOOTLOADER_MCUBOOT=y` from `sysbuild.conf`.

Add `CONFIG_TFM_PROFILE_TYPE_MINIMAL=y` to `prj.conf`.

This aligns exactly with the official NCS at_client sample pattern for nrf9151dk.

---

## Reference File Locations

| File | Path |
|------|------|
| NCS reference no-bootloader partitions | `/opt/nordic/ncs/v3.4.1/nrf/dts/samples/cellular/nrf91_no_bootloader_partitions.dtsi` |
| NCS reference SRAM partitions | `/opt/nordic/ncs/v3.4.1/nrf/dts/samples/cellular/nrf91_sram_partitions.dtsi` |
| NCS reference bootloader partitions | `/opt/nordic/ncs/v3.4.1/nrf/dts/samples/cellular/nrf91_bootloader_partitions.dtsi` |
| nrf91xx default partition layout | `/opt/nordic/ncs/v3.4.1/zephyr/dts/vendor/nordic/nrf91xx_partition.dtsi` |
| Nordic nrf9151dk reference NS overlay | `/opt/nordic/ncs/v3.4.1/nrf/samples/cellular/at_client/boards/nrf9151dk_nrf9151_ns.overlay` |
| NCS module DTS root declaration | `/opt/nordic/ncs/v3.4.1/nrf/zephyr/module.yml` (`dts_root: .`) |
