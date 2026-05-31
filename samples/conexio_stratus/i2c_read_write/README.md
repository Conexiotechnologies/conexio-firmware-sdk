# I2C Direct Read/Write — Conexio Stratus Pro

This sample demonstrates the core Zephyr I2C API functions using two real
onboard devices on the Stratus Pro I2C2 bus. It is intended as a practical
reference for any application that needs to talk to an I2C sensor or peripheral
directly, without a dedicated Zephyr driver.

---

## Hardware

| Bus  | Pins              | Speed    |
|------|-------------------|----------|
| I2C2 | SDA=P0.08, SCL=P0.07 | 400 kHz |

| Device           | Address | Used for                      |
|------------------|---------|-------------------------------|
| ST LIS2DH accel  | `0x18`  | Register read/write, burst read |
| Microchip 24CW160 EEPROM | `0x50` | Write then read back  |

Both devices are already defined in the board DTSI. The overlay only adds the
I2C pull-ups needed for reliable bus operation.

---

## API Covered

| Function              | What it does                                                   |
|-----------------------|----------------------------------------------------------------|
| `i2c_write_read_dt()` | Write N bytes (e.g. register address) then read M bytes back — most common sensor pattern |
| `i2c_write_dt()`      | Write N bytes (register address + data, or EEPROM address + payload) |
| `i2c_read_dt()`       | Read N bytes from the device's current internal pointer position |
| `i2c_transfer_dt()`   | Low-level: build a custom multi-message transaction with `struct i2c_msg` |

---

## Demos

### Demo 1 — `i2c_write_read_dt`: read a single register

Reads the LIS2DH `WHO_AM_I` register (0x0F). Always returns `0x33` — a
reliable sanity check that the device is alive and the bus is working.

```c
uint8_t reg = 0x0F;
uint8_t val = 0;
i2c_write_read_dt(&lis2dh, &reg, 1, &val, 1);
// val == 0x33
```

### Demo 2 — `i2c_write_dt`: write a configuration register

Enables the LIS2DH at 10 Hz with all three axes active by writing `CTRL_REG1`.

```c
uint8_t buf[2] = { 0x20, 0x27 };  /* reg addr, value */
i2c_write_dt(&lis2dh, buf, sizeof(buf));
```

### Demo 3 — `i2c_write_read_dt`: burst read

Reads 6 consecutive output registers (`OUT_X_L` through `OUT_Z_H`) in one
transaction, then reconstructs the three signed 16-bit accelerometer values.

```c
uint8_t reg = 0x28 | 0x80;   /* auto-increment bit set */
uint8_t raw[6];
i2c_write_read_dt(&lis2dh, &reg, 1, raw, 6);
int16_t x = (raw[1] << 8) | raw[0];
```

### Demo 4 — `i2c_write_dt` + `i2c_read_dt`: EEPROM write/read

Writes a 4-byte pattern (`0xDE 0xAD 0xBE 0xEF`) to EEPROM address `0x0010`,
then reads it back and verifies byte-by-byte. The 24CW160 uses 2-byte
addressing, so the write buffer is `[addr_hi, addr_lo, data...]`.

```c
uint8_t write_buf[] = { 0x00, 0x10, 0xDE, 0xAD, 0xBE, 0xEF };
i2c_write_dt(&eeprom, write_buf, sizeof(write_buf));
k_msleep(10);  /* wait for write cycle */

uint8_t addr[] = { 0x00, 0x10 };
i2c_write_dt(&eeprom, addr, sizeof(addr));   /* seek */
i2c_read_dt(&eeprom, readback, 4);           /* read */
```

### Demo 5 — `i2c_transfer_dt`: raw multi-message transaction

Manually builds the same write-then-read as Demo 1 using `struct i2c_msg`.
Use this when you need precise control over flags like `I2C_MSG_RESTART` or
want to chain more than two messages in one transaction.

```c
struct i2c_msg msgs[2] = {
    { .buf = &reg, .len = 1, .flags = I2C_MSG_WRITE },
    { .buf = &val, .len = 1, .flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP },
};
i2c_transfer_dt(&lis2dh, msgs, 2);
```

---

## Requirements

- Conexio Stratus Pro (nRF9151 or nRF9161)
- nRF Connect SDK v2.9.0 or newer

---

## Building and Running

**Stratus Pro nRF9151:**

```bash
west build -b conexio_stratus_pro/nrf9151/ns \
    -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

**Stratus Pro nRF9161:**

```bash
west build -b conexio_stratus_pro/nrf9161/ns \
    -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

Flash:

```bash
newtmgr -c serial image upload build/i2c_read_write/zephyr/zephyr.signed.bin
```

---

## Sample Output

```
I2C read/write sample running on conexio_stratus_pro_nrf9151_ns
[00:00:00.210,000] <inf> i2c_rw: I2C bus ready (SDA=P0.08, SCL=P0.07, 400 kHz)
[00:00:00.210,100] <inf> i2c_rw: --- Demo 1: i2c_write_read_dt (read WHO_AM_I) ---
[00:00:00.210,200] <inf> i2c_rw: LIS2DH WHO_AM_I = 0x33 (expected 0x33) — PASS
[00:00:00.210,300] <inf> i2c_rw: --- Demo 2: i2c_write_dt (configure LIS2DH) ---
[00:00:00.210,400] <inf> i2c_rw: LIS2DH CTRL_REG1 written (ODR=10Hz, XYZ enabled)
[00:00:00.410,000] <inf> i2c_rw: --- Demo 3: i2c_write_read_dt burst (6-byte accel data) ---
[00:00:00.420,000] <inf> i2c_rw: LIS2DH raw accel — X:   -512  Y:    320  Z:  15872 (LSB)
  raw bytes: FF 01 40 01 00 3E
[00:00:00.421,000] <inf> i2c_rw: --- Demo 4: i2c_write_dt + i2c_read_dt (EEPROM) ---
EEPROM wrote: DE AD BE EF
EEPROM read : DE AD BE EF
[00:00:00.435,000] <inf> i2c_rw: EEPROM write/read PASSED
[00:00:00.435,100] <inf> i2c_rw: --- Demo 5: i2c_transfer_dt (raw struct i2c_msg) ---
[00:00:00.435,200] <inf> i2c_rw: i2c_transfer_dt WHO_AM_I = 0x33 (raw transfer — same result)
[00:00:00.435,300] <inf> i2c_rw: I2C sample complete
```

---

## File Structure

```
i2c_read_write/
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay  # I2C pull-ups only
├── src/
│   └── main.c                                  # 5 demo functions
├── CMakeLists.txt
├── prj.conf
├── sysbuild.conf
└── README.md
```
