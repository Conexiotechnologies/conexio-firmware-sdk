# RTC Alarm Sample — Conexio Stratus Pro (nRF9151)

Demonstrates Zephyr's **counter API** as the RTC abstraction on the nRF9151.
Three progressive demos are included:

| Demo | Description                                   |
|------|-----------------------------------------------|
| 1    | One-shot alarm — 5 seconds                    |
| 2    | Periodic alarm — 3-second interval, 5 times   |
| 3    | Wake from system-off (opt-in Kconfig flag)     |

No external hardware required.

---

## The Counter API as RTC Abstraction

Zephyr does not expose the nRF RTC peripherals through a dedicated "RTC API".
Instead, the **counter API** (`<zephyr/drivers/counter.h>`) abstracts any
hardware timer — including the nRF LPCOMP RTC peripherals — behind a common
interface.

Key counter API functions used in this sample:

| Function                     | Purpose                                         |
|------------------------------|-------------------------------------------------|
| `counter_start()`            | Start the counter ticking                       |
| `counter_stop()`             | Stop the counter                                |
| `counter_get_frequency()`    | Query ticks per second (nRF RTC: 32768 Hz)      |
| `counter_get_top_value()`    | Query the counter wrap-around value             |
| `counter_us_to_ticks()`      | Convert microseconds to counter ticks           |
| `counter_set_channel_alarm()`| Set a one-shot alarm (absolute or relative)     |

### RTC Assignment on nRF9151

| RTC    | Reserved for          | Status in this sample |
|--------|-----------------------|-----------------------|
| RTC0   | Zephyr kernel tick    | Do not use            |
| RTC1   | Radio / BLE stack     | Do not use            |
| **RTC2** | **Application use** | **Enabled in overlay**|

The board overlay enables RTC2:

```dts
&rtc2 {
    status = "okay";
};
```

The device is accessed in code as:

```c
const struct device *rtc_dev = DEVICE_DT_GET(DT_NODELABEL(rtc2));
```

### Tick Frequency

The nRF RTC peripherals run at **32768 Hz** (the LFCLK crystal).
Converting seconds to ticks:

```c
uint32_t ticks = counter_us_to_ticks(rtc_dev, (uint64_t)seconds * 1000000ULL);
```

---

## Demo 1 — One-Shot Alarm

```
alarm set → 5 seconds pass → callback fires → k_sem_give() → main logs wakeup
```

The alarm callback runs in **ISR context**. Direct logging is not safe there,
so the callback only calls `k_sem_give()`. The main thread blocks on
`k_sem_take(K_FOREVER)` and logs after it wakes.

```c
/* Set the alarm */
struct counter_alarm_cfg alarm_cfg = {
    .callback  = alarm_callback,
    .ticks     = counter_us_to_ticks(rtc_dev, 5 * 1000000ULL),
    .user_data = NULL,
    .flags     = 0,
};
counter_set_channel_alarm(rtc_dev, 0, &alarm_cfg);

/* Wait for it */
k_sem_take(&alarm_sem, K_FOREVER);
```

---

## Demo 2 — Periodic Alarm

The counter API provides only one-shot alarms. A periodic alarm is built by
re-arming the alarm inside the handler loop:

```
set alarm → wait → alarm fires → log → re-arm → wait → alarm fires → ...
```

This is the recommended Zephyr pattern for periodic RTC alarms when
`counter_set_top_value_with_alarm()` is not suitable.

---

## Demo 3 — Wake from System-Off

Enable with `-DCONFIG_RTC_DEMO_SYSTEM_OFF=y`.

### Pattern

```
1. counter_start()           — ensure the RTC is running
2. counter_set_channel_alarm() — set alarm N seconds from now
3. sys_poweroff()            — enter SYSTEM OFF (≈0 µA, SRAM off)
4. ...RTC keeps running independently of the CPU...
5. Alarm fires → nRF9151 wakes → cold boot → MCUboot → main()
```

### Key Constraints

| Constraint | Explanation |
|------------|-------------|
| Set alarm **before** `sys_poweroff()` | After poweroff, the CPU is off |
| Cold boot on wake | No "resume"; `main()` runs from the start |
| SRAM is lost | All variables are re-initialized on wake |
| Retained RAM | Use `GPREGRET` / retained SRAM to pass state across the boundary |
| MCUboot runs again | The signed image must be valid for MCUboot to chain-load |

### Why SYSTEM OFF?

`sys_poweroff()` maps to the nRF9151 SYSTEM OFF state — the deepest
power mode, consuming only nanoamps. The RTC peripheral keeps running from
the always-on LFCLK domain, allowing timed wakeup without any CPU activity.

This is the recommended pattern for ultra-low-power periodic wake applications
(e.g., send a sensor reading every 10 minutes).

---

## Build and Flash

```bash
# Default (Demo 1 + Demo 2 only)
west build -b conexio_stratus_pro/nrf9151/ns -- \
    -DBOARD_ROOT=/opt/nordic/ncs/v2.9.0/conexio-firmware-sdk

# Include Demo 3 (wake from system-off)
west build -b conexio_stratus_pro/nrf9151/ns -- \
    -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk \
    -DCONFIG_RTC_DEMO_SYSTEM_OFF=y

# Flash via newtmgr (serial DFU)
newtmgr -c serial image upload build/rtc_alarm/zephyr/zephyr.signed.bin
```

---

## Expected Output

### Default build (Demo 1 + 2)

```
*** Booting Zephyr OS build v3.7.99-ncs2 ***
[00:00:00.210,000] <inf> rtc_alarm: === Conexio Stratus Pro — RTC Alarm Sample ===
[00:00:00.210,000] <inf> rtc_alarm: Board: conexio_stratus_pro/nrf9151/ns
[00:00:00.211,000] <inf> rtc_alarm: RTC device: rtc@4000b000
[00:00:00.211,000] <inf> rtc_alarm: Counter frequency : 32768 Hz
[00:00:00.211,000] <inf> rtc_alarm: Counter top value : 16777215 ticks
[00:00:00.211,000] <inf> rtc_alarm: Tick period       : ~30 us
[00:00:00.211,000] <inf> rtc_alarm: Counter started.
[00:00:00.211,000] <inf> rtc_alarm: --- Demo 1: One-shot alarm in 5 seconds ---
[00:00:00.211,000] <inf> rtc_alarm: One-shot alarm set. Waiting...
[00:00:05.212,000] <inf> rtc_alarm: RTC alarm fired! Uptime: 5212 ms
[00:00:05.212,000] <inf> rtc_alarm: Demo 1 complete.
[00:00:05.212,000] <inf> rtc_alarm: --- Demo 2: Periodic alarm every 3 s, 5 times ---
[00:00:08.213,000] <inf> rtc_alarm: Periodic alarm #1 fired! Uptime: 8213 ms
[00:00:11.214,000] <inf> rtc_alarm: Periodic alarm #2 fired! Uptime: 11214 ms
[00:00:14.215,000] <inf> rtc_alarm: Periodic alarm #3 fired! Uptime: 14215 ms
[00:00:17.216,000] <inf> rtc_alarm: Periodic alarm #4 fired! Uptime: 17216 ms
[00:00:20.217,000] <inf> rtc_alarm: Periodic alarm #5 fired! Uptime: 20217 ms
[00:00:20.217,000] <inf> rtc_alarm: Demo 2 complete (5 alarms fired).
[00:00:20.217,000] <inf> rtc_alarm: Counter stopped.
[00:00:20.217,000] <inf> rtc_alarm: Demo 3 (wake from system-off) is disabled.
[00:00:20.217,000] <inf> rtc_alarm: Build with -DCONFIG_RTC_DEMO_SYSTEM_OFF=y to enable it.
[00:00:20.217,000] <inf> rtc_alarm: All demos complete. Spinning.
```

### With `CONFIG_RTC_DEMO_SYSTEM_OFF=y`

```
[00:00:20.217,000] <inf> rtc_alarm: --- Demo 3: Wake from system-off in 10 seconds ---
[00:00:20.217,000] <inf> rtc_alarm: The RTC alarm will be set, then the system enters SYSTEM OFF.
[00:00:20.217,000] <inf> rtc_alarm: On wake, the nRF9151 cold-boots and main() runs from the start.
[00:00:20.217,000] <inf> rtc_alarm: All SRAM is lost; use retained RAM for cross-boot state.
[00:00:20.218,000] <inf> rtc_alarm: Wakeup alarm set for 10 s from now.
[00:00:20.218,000] <inf> rtc_alarm: Entering system-off now...
--- (system-off, ~0 µA, 10 seconds pass) ---
*** Booting Zephyr OS build v3.7.99-ncs2 ***
[00:00:00.210,000] <inf> rtc_alarm: === Conexio Stratus Pro — RTC Alarm Sample ===
```

---

## Author

Copyright (c) 2025 Conexio Technologies, Inc.  
Rajeev Piyare \<rajeev@conexiotech.com\>  
SPDX-License-Identifier: Apache-2.0
