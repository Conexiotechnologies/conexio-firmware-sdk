# NVS Storage — Conexio Stratus Pro

NVS (Non-Volatile Storage) is a lightweight key-value store backed by internal
flash. Data written to NVS survives power cycles and reboots, making it the
standard way to persist configuration, device identity, and application state
on nRF91 devices.

---

## What the Sample Demonstrates

| Demo | What it shows |
|------|---------------|
| 1 — Boot counter | Read-modify-write a `uint32_t` that increments on every boot |
| 2 — String storage | Store and update a null-terminated device name string |
| 3 — Structured config | Persist a C struct (factory defaults → loaded on subsequent boots) |
| 4 — Delete | `nvs_delete()` and confirming `-ENOENT` on next read |
| 5 — Binary blob | Store a raw byte array (e.g. a device secret or crypto key) |

---

## How NVS Works

NVS stores entries as `(id, data)` pairs in a dedicated flash partition. Each
entry has a 16-bit ID chosen by the application. The storage is append-only at
the flash level — updates write a new version of the entry and the old one is
reclaimed during garbage collection.

```
Flash layout (storage_partition):
  ┌────────────────────────────┐  0xfa000
  │ Sector 0 (4 KiB)           │  active writes
  ├────────────────────────────┤  0xfb000
  │ Sector 1 (4 KiB)           │  overflow / GC target
  ├────────────────────────────┤  0xfc000
  │ Sector 2 (4 KiB)           │  spare for GC
  └────────────────────────────┘  0xfd000
```

NVS needs at least 2 sectors: one active, one for garbage collection. 3 sectors
gives better write endurance distribution.

---

## API

```c
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>

// Mount (call once per boot, before any read/write)
nvs_mount(&nvs);

// Write (returns bytes written, 0 if data unchanged, negative on error)
nvs_write(&nvs, NVS_ID_BOOT_COUNT, &value, sizeof(value));

// Read (returns bytes read, -ENOENT if not found, negative on other error)
nvs_read(&nvs, NVS_ID_BOOT_COUNT, &value, sizeof(value));

// Delete (next read returns -ENOENT)
nvs_delete(&nvs, NVS_ID_BOOT_COUNT);

// Check remaining space
ssize_t free = nvs_calc_free_space(&nvs);
```

---

## Key ID Rules

- IDs are `uint16_t` values chosen by the application
- **Never reuse or renumber an ID once deployed** — existing stored data would
  be misread as the wrong type
- IDs `0x0000` and `0xFFFF` are reserved by NVS — do not use them
- Define IDs as an `enum` to prevent accidental collisions:

```c
enum nvs_key_id {
    NVS_ID_BOOT_COUNT  = 1,
    NVS_ID_DEVICE_NAME = 2,
    NVS_ID_CONFIG      = 3,
};
```

---

## Flash Write Wear

NVS uses append-only writes and garbage collects when a sector fills up.
On the nRF9151 the internal flash is rated for ~10,000 erase cycles per block.

- A boot counter that writes on every boot will wear the sector after ~10,000
  boots — acceptable for most deployments.
- For high-frequency writes (e.g. logging sensor data every second), use a
  larger sector count or an external flash with higher endurance.
- `nvs_write` is a **no-op if the data hasn't changed** (returns 0) — take
  advantage of this by reading before writing when the value may be the same.

---

## Requirements

- Conexio Stratus Pro (nRF9151 or nRF9161)
- nRF Connect SDK v3.2.1 or newer

---

## Building and Running

```bash
west build -b conexio_stratus_pro/nrf9151/ns \
    -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

Flash:

```bash
newtmgr -c serial image upload build/nvs_storage/zephyr/zephyr.signed.bin
```

Reset the board a few times to watch the boot counter increment.

---

## Sample Output

**First boot** (empty NVS):

```
========================================
  Conexio Stratus Pro — NVS Storage
  Board: conexio_stratus_pro_nrf9151_ns
========================================

[00:00:00.310,000] <inf> nvs_storage: NVS mounted successfully
[00:00:00.310,100] <inf> nvs_storage:   Flash device : flash-controller@39000
[00:00:00.310,200] <inf> nvs_storage:   Offset       : 0x000FA000
[00:00:00.310,300] <inf> nvs_storage:   Sector size  : 4096 bytes
[00:00:00.310,400] <inf> nvs_storage:   Sector count : 3
[00:00:00.310,500] <inf> nvs_storage:   Total NVS    : 12288 bytes
[00:00:00.310,600] <inf> nvs_storage:   Free space   : 12192 bytes
[00:00:00.311,000] <inf> nvs_storage: --- Demo 1: Boot counter ---
[00:00:00.311,100] <inf> nvs_storage: No boot count found (first boot) — initialising to 1
[00:00:00.311,200] <inf> nvs_storage: Boot count saved: 1 (4 bytes written)
[00:00:00.312,000] <inf> nvs_storage: --- Demo 2: String storage ---
[00:00:00.312,100] <inf> nvs_storage: Device name initialised: "Stratus-Pro-001"
[00:00:00.313,000] <inf> nvs_storage: --- Demo 3: Structured config ---
[00:00:00.313,100] <inf> nvs_storage: Config initialised with factory defaults
[00:00:00.314,000] <inf> nvs_storage: --- Demo 4: Delete ---
[00:00:00.314,100] <inf> nvs_storage: Wrote threshold=42 (4 bytes)
[00:00:00.314,200] <inf> nvs_storage: Read back threshold=42 (4 bytes)
[00:00:00.314,300] <inf> nvs_storage: Deleted threshold entry (ret=0)
[00:00:00.314,400] <inf> nvs_storage: Confirmed: threshold entry is gone (-ENOENT)
[00:00:00.315,000] <inf> nvs_storage: --- Demo 5: Binary blob ---
[00:00:00.315,100] <inf> nvs_storage: Secret written:
                                      de ad be ef ca fe ba be          |........|
[00:00:00.316,000] <inf> nvs_storage: --- Summary ---
[00:00:00.316,100] <inf> nvs_storage: NVS free space remaining: 12080 bytes
[00:00:00.316,200] <inf> nvs_storage: NVS sample complete. Reset the board to see boot count increment.
```

**Second boot** (data already stored):

```
[00:00:00.311,000] <inf> nvs_storage: --- Demo 1: Boot counter ---
[00:00:00.311,100] <inf> nvs_storage: Previous boot count: 1 — incrementing
[00:00:00.311,200] <inf> nvs_storage: Boot count saved: 2 (4 bytes written)
[00:00:00.312,000] <inf> nvs_storage: --- Demo 2: String storage ---
[00:00:00.312,100] <inf> nvs_storage: Stored device name: "Stratus-Pro-001" (16 bytes)
[00:00:00.312,200] <inf> nvs_storage: Device name updated to: "Stratus-Pro-002"
[00:00:00.313,000] <inf> nvs_storage: --- Demo 3: Structured config ---
[00:00:00.313,100] <inf> nvs_storage: Loaded config:
[00:00:00.313,200] <inf> nvs_storage:   sample_interval_ms : 5000
[00:00:00.313,300] <inf> nvs_storage:   alert_threshold    : 75
[00:00:00.313,400] <inf> nvs_storage:   flags              : 0x01 (LED=on, verbose=no)
```

---

## File Structure

```
nvs_storage/
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay  # Points NVS at storage_partition
├── src/
│   └── main.c                                  # 5 demo functions
├── CMakeLists.txt
├── prj.conf                                    # CONFIG_NVS, FLASH, FLASH_MAP
├── sysbuild.conf
└── README.md
```
