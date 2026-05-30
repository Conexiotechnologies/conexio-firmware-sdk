# nPM1300 LEDs Sample — Conexio Stratus Pro

This sample demonstrates how to control the three LED outputs of the Nordic nPM1300 PMIC on the
**Conexio Stratus Pro (nRF9151)** using the Zephyr LED driver API.

---

## Overview

The nPM1300 PMIC includes three LED driver outputs, each configurable in one of three modes:

| Mode       | Behaviour                                                        |
|------------|------------------------------------------------------------------|
| `charging` | Automatically driven by the nPM1300 battery charger state machine |
| `error`    | Automatically driven by internal PMIC error conditions           |
| `host`     | Fully software-controlled via the Zephyr `led_on()` / `led_off()` API |

This sample configures the LEDs as follows:

| LED  | Mode       | Control        | Description                          |
|------|------------|----------------|--------------------------------------|
| LED0 | `charging` | Automatic      | Reflects battery charging status     |
| LED1 | `error`    | Automatic      | Reflects PMIC error state            |
| LED2 | `host`     | Software (you) | General-purpose user status LED      |

On startup the sample:
1. Runs a one-shot **chase sequence** across LED0 → LED1 → LED2 (useful for board bring-up).
   LED0 and LED1 are in auto mode so software calls on them are gracefully ignored.
2. Enters a continuous **blink loop** on **LED2** (host mode) at a configurable interval.

---

## Key Functions

### `user_led_set(bool on)`

The dedicated function for controlling LED2 from application code. Use this in your own
application to turn the user LED on or off as a status indicator:

```c
user_led_set(true);   // Turn LED2 on
user_led_set(false);  // Turn LED2 off
```

### `led_chase_sequence()`

Sequences all three LED outputs on then off, one at a time. Useful during bring-up to
visually confirm the LED wiring is correct.

---

## Configuration

The blink interval is configurable via Kconfig:

```
CONFIG_NPM_LEDS_BLINK_INTERVAL_MS=500   # default: 500 ms
```

You can override this in `prj.conf` or on the west build command line.

---

## Requirements

- Conexio Stratus Pro board with nRF9151 SoC
- nPM1300 PMIC (on-board, connected via I2C2 at address `0x6B`)
- nRF Connect SDK v2.9.0

---

## Building and Running

Build for the Stratus Pro nRF9151 target:

```bash
west build -b conexio_stratus_pro/nrf9151/ns \
    -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

Flash the firmware using `newtmgr`:

```bash
newtmgr -c serial image upload build/npm_leds/zephyr/zephyr.signed.bin
```

---

## Sample Output

```
*** Booting nRF Connect SDK v3.2.1 ***
nPM1300 LED sample running on conexio_stratus_pro_nrf9151_ns
[00:00:00.210,000] <inf> npm_leds: nPM1300 LED device ready
[00:00:00.210,100] <inf> npm_leds: LED0 = charging (auto), LED1 = error (auto), LED2 = host (software)
[00:00:00.210,200] <inf> npm_leds: --- Chase sequence: LED0 -> LED1 -> LED2 ---
[00:00:00.210,300] <inf> npm_leds: LED0 is in auto mode (charging/error), skipping software control
[00:00:00.711,000] <inf> npm_leds: LED1 is in auto mode (charging/error), skipping software control
[00:00:01.212,000] <inf> npm_leds: LED2 ON
[00:00:01.712,000] <inf> npm_leds: LED2 OFF
[00:00:02.713,000] <inf> npm_leds: Starting continuous blink on LED2 (host mode)
[00:00:02.713,100] <inf> npm_leds: LED2 ON
[00:00:03.213,000] <inf> npm_leds: LED2 OFF
[00:00:03.713,000] <inf> npm_leds: LED2 ON
[00:00:04.213,000] <inf> npm_leds: LED2 OFF
```

> **Note:** LED0 and LED1 log "auto mode" messages because software calls on `charging` and
> `error` mode LEDs return `-ENOTSUP` by design — the PMIC hardware manages them directly.

---

## File Structure

```
npm_leds/
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay  # Adds npm1300_leds DT node to pmic_main
├── src/
│   └── main.c                                  # Application code
├── CMakeLists.txt
├── Kconfig                                     # NPM_LEDS_BLINK_INTERVAL_MS option
├── prj.conf                                    # Enables LED, MFD, regulator drivers
├── sysbuild.conf                               # MCUboot enabled
└── README.md
```

---

## How It Works

The nPM1300 LED controller is exposed in the devicetree as a child node of `pmic_main`
(the nPM1300 MFD device on I2C2). The overlay adds:

```dts
&pmic_main {
    npm1300_leds: leds {
        compatible = "nordic,npm1300-led";
        nordic,led0-mode = "charging";
        nordic,led1-mode = "error";
        nordic,led2-mode = "host";
    };
};
```

The Zephyr `LED_NPM1300` driver auto-enables when this node is present and programs each LED's
mode register at init time. From that point, `charging` and `error` LEDs are managed by the
PMIC hardware, while the `host` LED is available for application control via the standard
Zephyr LED API (`led_on` / `led_off`).
