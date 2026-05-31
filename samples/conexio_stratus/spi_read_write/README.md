# SPI Direct Read/Write — Conexio Stratus Pro

This sample demonstrates the core Zephyr SPI API functions using the SPI3 bus
exposed on the Stratus Pro edge connector. It is the SPI counterpart to the
`i2c_read_write` sample and is intended as a practical reference for any
application that needs to drive an SPI sensor or peripheral directly.

---

## Hardware

| Bus  | Pin    | Header | Direction |
|------|--------|--------|-----------|
| SPI3 | P0.13  | D19    | SCK       |
| SPI3 | P0.14  | D20    | MOSI      |
| SPI3 | P0.15  | D21    | MISO      |
| SPI3 | P0.17  | D22    | CS (active-low, hardware-controlled) |

Connect your SPI device to these four header pins. The CS line is driven
automatically by the nRF SPIM3 hardware before and after each transaction.

**No external device needed for Demo 6 (loopback)** — just bridge D20 (MOSI)
to D21 (MISO) with a jumper wire.

---

## SPI Configuration

Configured in the overlay (`boards/conexio_stratus_pro_nrf9151_ns.overlay`):

| Parameter     | Value   | Notes                                  |
|---------------|---------|----------------------------------------|
| Speed         | 1 MHz   | Change `spi-max-frequency` for faster devices |
| Mode          | Mode 0  | CPOL=0, CPHA=0 — most sensors default  |
| Word size     | 8-bit   |                                        |
| Bit order     | MSB first |                                      |

To use a different SPI mode, add `spi-cpol;` and/or `spi-cpha;` properties
to the `spi-device@0` node in the overlay.

---

## How SPI Transfers Work in Zephyr

All SPI transfers use two structures:

```c
struct spi_buf     { void *buf; size_t len; }
struct spi_buf_set { const struct spi_buf *buffers; size_t count; }
```

TX and RX buffer sets are passed independently. Either can be `NULL` for
direction-only transfers. Multiple `spi_buf` entries in a set are transmitted
sequentially within one CS assertion (scatter-gather).

---

## API Covered

| Function             | What it does                                                       |
|----------------------|--------------------------------------------------------------------|
| `spi_write_dt()`     | TX-only: shifts out bytes, ignores MISO                            |
| `spi_read_dt()`      | RX-only: clocks in bytes, MOSI sends 0x00 dummy bytes              |
| `spi_transceive_dt()`| Full-duplex: simultaneously shifts TX and captures RX              |

All three are wrappers around the same underlying `spi_transceive()` call.

---

## Demos

### Demo 1 — `spi_write_dt`: TX-only transfer

Sends a 3-byte command/address sequence. MISO is ignored. Use for write
commands, display command bytes, or any case where the device response
during that phase doesn't matter.

```c
uint8_t tx[] = { 0x06, 0x00, 0xFF };
const struct spi_buf tx_buf   = { .buf = tx, .len = sizeof(tx) };
const struct spi_buf_set tx_s = { .buffers = &tx_buf, .count = 1 };
spi_write_dt(&spidev, &tx_s);
```

### Demo 2 — `spi_read_dt`: RX-only transfer

Clocks in 4 bytes while MOSI sends dummy `0x00` bytes. Use after a
prior write has set up the device to respond.

```c
uint8_t rx[4] = {0};
const struct spi_buf rx_buf   = { .buf = rx, .len = sizeof(rx) };
const struct spi_buf_set rx_s = { .buffers = &rx_buf, .count = 1 };
spi_read_dt(&spidev, &rx_s);
```

### Demo 3 — `spi_transceive_dt`: full-duplex, single buffer pair

Sends a command byte + dummies while capturing the device response.
This is the standard pattern for reading a sensor register over SPI.

```c
uint8_t tx[] = { 0x8F, 0x00, 0x00 };  /* cmd + dummy bytes */
uint8_t rx[sizeof(tx)] = {0};
/* set up buf/buf_set for both, then: */
spi_transceive_dt(&spidev, &tx_s, &rx_s);
/* rx[1], rx[2] now hold the response */
```

### Demo 4 — `spi_transceive_dt`: split command + response buffers

Uses two `spi_buf` entries per direction — one for the command phase
and one for the data phase — all within a single CS assertion.

```c
uint8_t cmd[]      = { 0x03, 0x00 };   /* read cmd + address */
uint8_t rx_ignore[2] = {0};            /* discard cmd echo   */
uint8_t rx_data[4]   = {0};            /* capture payload    */
/* TX: cmd + dummy; RX: ignore + data */
spi_transceive_dt(&spidev, &tx_s, &rx_s);
```

### Demo 5 — `spi_transceive_dt`: scatter-gather multi-buffer

Transmits a packet built from three separate memory regions (header,
payload, footer) in one CS assertion — no memcpy required.

```c
const struct spi_buf tx_bufs[] = {
    { .buf = header,  .len = 2 },
    { .buf = payload, .len = 4 },
    { .buf = footer,  .len = 1 },
};
const struct spi_buf_set tx = { .buffers = tx_bufs, .count = 3 };
spi_transceive_dt(&spidev, &tx, &rx_s);
```

### Demo 6 — Loopback self-test

**Requires: bridge D20 (MOSI) → D21 (MISO) with a jumper wire.**

Sends a known 8-byte pattern and verifies the received bytes match
exactly. No external device needed. Use this to confirm the SPI bus,
pinctrl, and CS GPIO are all wired and configured correctly.

---

## Requirements

- Conexio Stratus Pro (nRF9151 or nRF9161)
- nRF Connect SDK v3.2.1 or newer
- Jumper wire (D20 → D21) for Demo 6 loopback

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
newtmgr -c serial image upload build/spi_read_write/zephyr/zephyr.signed.bin
```

---

## Sample Output

With loopback wire present:

```
SPI read/write sample running on conexio_stratus_pro_nrf9151_ns
[00:00:00.211,000] <inf> spi_rw: SPI bus ready
[00:00:00.211,100] <inf> spi_rw:   Bus:  SPI3 (nRF SPIM3)
[00:00:00.211,200] <inf> spi_rw:   SCK   P0.13 (D19)
[00:00:00.211,300] <inf> spi_rw:   MOSI  P0.14 (D20)
[00:00:00.211,400] <inf> spi_rw:   MISO  P0.15 (D21)
[00:00:00.211,500] <inf> spi_rw:   CS    P0.17 (D22) — active-low, hardware-controlled
[00:00:00.211,600] <inf> spi_rw:   Speed 1000000 Hz, Mode 0 (CPOL=0, CPHA=0)
[00:00:00.211,700] <inf> spi_rw: --- Demo 1: spi_write_dt (TX-only) ---
TX (write-only) (3 bytes): 06 00 FF
[00:00:00.212,000] <inf> spi_rw: spi_write_dt OK
[00:00:00.212,100] <inf> spi_rw: --- Demo 2: spi_read_dt (RX-only) ---
RX (read-only) (4 bytes): 00 00 00 00
[00:00:00.212,400] <inf> spi_rw: spi_read_dt OK (data valid only with real device)
...
[00:00:00.215,000] <inf> spi_rw: --- Demo 6: Loopback self-test (bridge D20→D21 required) ---
TX (sent) (8 bytes): 01 23 45 67 89 AB CD EF
RX (received) (8 bytes): 01 23 45 67 89 AB CD EF
[00:00:00.216,000] <inf> spi_rw: Loopback PASSED — TX matches RX
[00:00:00.216,100] <inf> spi_rw: SPI sample complete
```

---

## Connecting a Real SPI Device

1. Wire your device to D19/D20/D21/D22 on the Stratus header.
2. Update `spi-max-frequency` in the overlay to match your device.
3. Add `spi-cpol;` / `spi-cpha;` to the overlay node if your device
   uses SPI Mode 1, 2, or 3.
4. Replace the placeholder bytes in Demo 3/4 with your device's actual
   register addresses and command bytes.

---

## File Structure

```
spi_read_write/
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay  # Enables SPI3, defines spi_dev0
├── src/
│   └── main.c                                  # 6 demo functions
├── CMakeLists.txt
├── prj.conf
├── sysbuild.conf
└── README.md
```
