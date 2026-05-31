# External EEPROM (24CW160) Sample — Conexio Stratus Pro

This sample demonstrates reading from and writing to the onboard **Microchip 24CW160**
I2C EEPROM on the Conexio Stratus Pro (nRF9151) using the Zephyr EEPROM driver API.

---

## Hardware

| Property     | Value                          |
|--------------|--------------------------------|
| IC           | Microchip 24CW160              |
| Capacity     | 2 KiB (16 kbit)                |
| Page size    | 32 bytes                       |
| Interface    | I2C2 (SDA=P0.08, SCL=P0.07)   |
| I2C address  | 0x50                           |
| Write cycle  | 5 ms max                       |

The EEPROM is powered by the onboard sensor power rail (GPIO 26) which is enabled
at boot via the `sensor-pwr-ctrl` regulator node in the board DTSI.

---

## What the Sample Does

1. Queries the EEPROM size using `eeprom_get_size()`
2. Writes a rolling test pattern (`0xA0, 0xA1, 0xA2, ...`) using `eeprom_write()`
3. Reads the data back using `eeprom_read()`
4. Verifies byte-by-byte that the read data matches what was written
5. Erases the test region (writes zeros) and verifies the erase

---

## Configuration

| Kconfig option                  | Default | Description                                    |
|---------------------------------|---------|------------------------------------------------|
| `CONFIG_EEPROM_TEST_OFFSET`     | `0`     | Byte offset within EEPROM to start the test    |
| `CONFIG_EEPROM_TEST_PATTERN_SIZE` | `32`  | Number of bytes to write/read in the test      |

Override in `prj.conf` if needed, e.g.:

```
CONFIG_EEPROM_TEST_OFFSET=64
CONFIG_EEPROM_TEST_PATTERN_SIZE=64
```

---

## Overlay Notes

The `eeprom0` node is defined in the board DTSI — no additional DT nodes are required.
The overlay (`boards/conexio_stratus_pro_nrf9151_ns.overlay`) only enables the I2C2
bus pull-ups, which are needed for reliable communication with all devices on that bus.

---

## Requirements

- Conexio Stratus Pro board with nRF9151 SoC
- nRF Connect SDK v3.2.1 or newer

---

## Building and Running

```bash
west build -b conexio_stratus_pro/nrf9151/ns \
    -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

Flash:

```bash
newtmgr -c serial image upload build/ext_flash/zephyr/zephyr.signed.bin
```

---

## Sample Output

```
EEPROM sample running on conexio_stratus_pro_nrf9151_ns
[00:00:00.312,000] <inf> ext_flash: EEPROM device: eeprom@50
[00:00:00.312,100] <inf> ext_flash: EEPROM size: 2048 bytes
Write buffer (32 bytes):
A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF
B0 B1 B2 B3 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF
[00:00:00.313,000] <inf> ext_flash: Writing 32 bytes to EEPROM offset 0x0000...
[00:00:00.320,000] <inf> ext_flash: Write OK
[00:00:00.330,000] <inf> ext_flash: Reading 32 bytes from EEPROM offset 0x0000...
[00:00:00.332,000] <inf> ext_flash: Read OK
Read buffer (32 bytes):
A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF
B0 B1 B2 B3 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF
[00:00:00.332,100] <inf> ext_flash: Verify PASSED — read data matches written data
[00:00:00.332,200] <inf> ext_flash: Erasing test region (writing zeros)...
[00:00:00.343,000] <inf> ext_flash: Erase verify PASSED — region is zeroed
[00:00:00.343,100] <inf> ext_flash: EEPROM sample complete
```

---

## API Used

| Function           | Description                              |
|--------------------|------------------------------------------|
| `eeprom_get_size()` | Returns total EEPROM capacity in bytes  |
| `eeprom_write()`   | Writes bytes at a given offset           |
| `eeprom_read()`    | Reads bytes from a given offset          |

All functions are from `<zephyr/drivers/eeprom.h>`.

---

## File Structure

```
ext_flash/
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay  # I2C pull-ups only
├── src/
│   └── main.c                                  # Application code
├── CMakeLists.txt
├── Kconfig                                     # Test offset/size options
├── prj.conf                                    # Enables EEPROM_AT24 driver
├── sysbuild.conf                               # MCUboot enabled
└── README.md
```
