# Watchdog Sample — Conexio Stratus Pro (nRF9151)

Demonstrates the Zephyr Watchdog Timer (WDT) API on the nRF9151 SoC.

Features:
* Installing a 5-second WDT timeout with a last-gasp callback
* Periodic feeding from a dedicated `watchdog_feed_thread` (every 2 s)
* Button-triggered feed pause to demonstrate live watchdog reset
* Optional automatic reset demo via `CONFIG_WDT_DEMO_TRIGGER_RESET`

---

## How the Watchdog Works

The nRF9151 WDT is a hardware counter that counts down independently of the
CPU. If the application does not "feed" (reset) the counter before it reaches
zero, the hardware resets the entire SoC.

```
WDT timeout = 5000 ms
Feed interval = 2000 ms

Timeline (normal operation):
  t=0s  WDT started          counter starts at 5 s
  t=2s  feed #1              counter reset to 5 s
  t=4s  feed #2              counter reset to 5 s
  t=6s  feed #3              ...

Timeline (feed paused at t=4s):
  t=4s  button pressed       feeding stops
  t=5s  counter reaches 0    callback fires ("WDT expiry callback!")
  t=5s  SoC resets           MCUboot re-runs, application restarts
```

### Window Timeouts

`wdt_install_timeout()` accepts a `window` with both `min` and `max`:

| Field        | This sample | Meaning                              |
|--------------|-------------|--------------------------------------|
| `window.min` | 0 ms        | Can feed at any time (no early limit)|
| `window.max` | 5000 ms     | Must feed before 5 s or reset fires  |

Some applications set `window.min > 0` to catch runaway loops that feed
the WDT too early (indicating the scheduler is stuck).

### Callback vs Reset

The `wdt_callback` is a **last-gasp** notification — it runs in ISR context
just before the hardware reset. Use it to:
* Flush a log buffer to non-volatile storage
* Signal an external component that a reset is imminent
* Write a reset reason to retained RAM

The callback **cannot** prevent the reset. Keep it short and non-blocking.

### `WDT_OPT_PAUSE_IN_SLEEP`

Pauses the WDT hardware counter while the CPU is in a low-power sleep state
(`k_msleep`, tickless idle). Without this option, the WDT would expire during
sleep even if the application is working correctly.

Use this option when your feed interval is shorter than the time the CPU
spends sleeping. Without it, set `window.max` larger than your longest
expected sleep duration.

### `WDT_OPT_PAUSE_HALTED_BY_DBG`

Pauses the WDT when a JTAG/SWD debugger halts the CPU at a breakpoint.
Always set this during development — otherwise the WDT fires every 5 seconds
while you're inspecting variables.

---

## Practical Feeding Strategies

| Strategy              | When to use                                      |
|-----------------------|--------------------------------------------------|
| Dedicated thread      | This sample — simple, predictable interval       |
| Main loop             | Single-threaded apps; feed at top of loop        |
| System workqueue      | When the workqueue itself is the health signal   |
| Multiple channels     | Feed only when all subsystems are healthy        |

For production, feed the WDT only after all critical subsystems have
completed their work cycle. A WDT fed unconditionally from a timer is
better than nothing but won't catch deadlocks in subsystem threads.

---

## Build and Flash

```bash
# Normal operation (feed indefinitely; button pauses feeding)
west build -b conexio_stratus_pro/nrf9151/ns -- \
    -DBOARD_ROOT=/opt/nordic/ncs/v2.9.0/conexio-firmware-sdk

# Demo mode (automatically stops feeding after 5 cycles)
west build -b conexio_stratus_pro/nrf9151/ns -- \
    -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk \
    -DCONFIG_WDT_DEMO_TRIGGER_RESET=y

# Flash via newtmgr (serial DFU)
newtmgr -c serial image upload build/watchdog/zephyr/zephyr.signed.bin
```

---

## Expected Output

### Normal boot and feeding

```
*** Booting Zephyr OS build v3.7.99-ncs2 ***
[00:00:00.215,000] <inf> watchdog_sample: === Conexio Stratus Pro — Watchdog Sample ===
[00:00:00.215,000] <inf> watchdog_sample: System restarted. Uptime: 215 ms
[00:00:00.215,000] <inf> watchdog_sample: Board: conexio_stratus_pro/nrf9151/ns
[00:00:00.215,000] <inf> watchdog_sample: WDT device: wdt@40010000
[00:00:00.216,000] <inf> watchdog_sample: WDT timeout installed: channel 0, max=5000 ms
[00:00:00.216,000] <inf> watchdog_sample: WDT started (5 s timeout, pause-in-sleep, pause-halted-by-dbg).
[00:00:00.216,000] <inf> watchdog_sample: Button configured (gpio0 pin 31). Press to pause WDT feed.
[00:00:00.216,000] <inf> watchdog_sample: Normal mode: WDT fed indefinitely every 2000 ms.
[00:00:00.216,000] <inf> watchdog_sample: Watchdog feed thread started (feed interval: 2000 ms, WDT timeout: 5000 ms)
[00:00:02.217,000] <inf> watchdog_sample: WDT fed (count: 1, uptime: 2217 ms)
[00:00:04.218,000] <inf> watchdog_sample: WDT fed (count: 2, uptime: 4218 ms)
```

### After pressing the MODE button

```
WDT feed paused — system will reset in ~5s
[00:00:12.300,000] <wrn> watchdog_sample: WDT feed paused — reset in ~5 s
[00:00:13.301,000] <wrn> watchdog_sample: WDT feed paused — reset in ~4 s
[00:00:14.302,000] <wrn> watchdog_sample: WDT feed paused — reset in ~3 s
[00:00:15.303,000] <wrn> watchdog_sample: WDT feed paused — reset in ~2 s
[00:00:16.304,000] <wrn> watchdog_sample: WDT feed paused — reset in ~1 s
WDT expiry callback! System will reset.
*** Booting Zephyr OS build v3.7.99-ncs2 ***
[00:00:00.215,000] <inf> watchdog_sample: System restarted. Uptime: 215 ms
```

### Demo mode (`CONFIG_WDT_DEMO_TRIGGER_RESET=y`)

```
[00:00:00.216,000] <inf> watchdog_sample: Demo mode enabled: WDT feed will stop after 5 feeds (~10 s).
[00:00:02.217,000] <inf> watchdog_sample: WDT fed (count: 1, uptime: 2217 ms)
...
[00:00:10.221,000] <inf> watchdog_sample: WDT fed (count: 5, uptime: 10221 ms)
[00:00:10.221,000] <wrn> watchdog_sample: Demo mode: stopping WDT feed after 5 feeds.
[00:00:10.221,000] <wrn> watchdog_sample: System will reset in ~5 seconds...
WDT expiry callback! System will reset.
*** Booting Zephyr OS build v3.7.99-ncs2 ***
```

---

## Author

Copyright (c) 2025 Conexio Technologies, Inc.  
Rajeev Piyare \<rajeev@conexiotech.com\>  
SPDX-License-Identifier: Apache-2.0
