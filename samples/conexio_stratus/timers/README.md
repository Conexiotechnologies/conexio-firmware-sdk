# Timers Sample — Conexio Stratus Pro

**Author:** Rajeev Piyare &lt;rajeev@conexiotech.com&gt;  
**Copyright:** Copyright (c) 2026 Conexio Technologies, Inc.  
**License:** Apache-2.0

## Overview

This sample is a complete educational tour of Zephyr kernel timers on the Conexio Stratus Pro (nRF9151). It covers one-shot and periodic timers, expiry and stop callbacks, polling with `k_timer_status_get`, blocking waits with `k_timer_status_sync`, and the critical ISR context rules that govern what you can do inside a timer callback.

## Timer Concepts Demonstrated

### 1. `K_TIMER_DEFINE` — Static timer declaration

All three timers in this sample are declared at compile time. The macro allocates the `struct k_timer` in static storage and registers optional expiry and stop callbacks.

```c
K_TIMER_DEFINE(my_timer, expiry_fn, stop_fn);
//                        ^^^^^^^^  ^^^^^^^
//                        called on expiry  called when stopped early
// Pass NULL for either callback if not needed.
```

### 2. One-shot Timer

Fires a single time after the specified duration and then stops.

```c
// duration = 3000 ms, period = K_NO_WAIT (no repeat)
k_timer_start(&oneshot_timer, K_MSEC(3000), K_NO_WAIT);
```

The timer becomes idle after the first expiry. You can re-arm it with another `k_timer_start` call.

### 3. Periodic Timer

Fires repeatedly at a fixed interval. The LED is toggled on each expiry.

```c
// duration = 1000 ms (first fire), period = 1000 ms (every subsequent)
k_timer_start(&periodic_timer, K_MSEC(1000), K_MSEC(1000));
```

To stop it: `k_timer_stop(&periodic_timer)`.

### 4. Timer with Stop Callback

The stop callback fires when `k_timer_stop()` is called on a running timer. It does **not** fire when the timer expires normally.

```c
K_TIMER_DEFINE(watchdog_timer, watchdog_expiry_fn, watchdog_stop_fn);

// ...later...
k_timer_stop(&watchdog_timer);  // → watchdog_stop_fn() is called
```

Use the stop callback for cleanup, state machine transitions, or logging that a watchdog was cancelled before it could bite.

### 5. `k_timer_status_get()` — Non-blocking expiry count

Returns the number of times the timer has expired since the last call (or since `k_timer_start`). **Resets the counter on each call** (consume, not peek). Safe from any context including ISR.

```c
uint32_t count = k_timer_status_get(&periodic_timer);
// count = number of expirations since last call; counter now reset to 0
```

### 6. `k_timer_remaining_get()` — Time until next expiry

Returns milliseconds until the next expiry. Returns 0 if the timer is stopped or has already expired (one-shot). Safe from any context.

```c
uint32_t ms = k_timer_remaining_get(&oneshot_timer);
```

### 7. `k_timer_stop()` — Cancel a running timer

```c
k_timer_stop(&watchdog_timer);
```

If the timer has a stop callback, it is called synchronously before `k_timer_stop` returns.

### 8. `k_timer_status_sync()` — Blocking wait pattern

Blocks the calling **thread** until the timer fires at least once since the last sync (or since `k_timer_start`). Returns the number of expirations that occurred while waiting. This is the cleanest way to synchronise a thread to a periodic timer.

```c
// Block until the next periodic_timer expiry:
uint32_t n = k_timer_status_sync(&periodic_timer);
// n = number of times it fired while we waited
```

> `k_timer_status_sync` can only be called from **thread context** — not from an ISR or another timer callback.

### 9. LED Blink from Timer Callback

The periodic timer toggles the LED in its expiry callback using `gpio_pin_toggle_dt`. Fast GPIO operations are ISR-safe on nRF devices. For complex sequences, defer to a work queue instead.

### 10. Callback vs Polling

| | Callback (expiry/stop fn) | Polling (thread context) |
|---|---|---|
| Context | **Interrupt (ISR)** | Thread |
| `LOG_INF` | ❌ Not safe | ✅ Safe |
| `k_msleep` | ❌ Not safe | ✅ Safe |
| `printk` | ✅ Safe | ✅ Safe |
| `k_work_submit` | ✅ Safe | ✅ Safe |
| `atomic_inc` | ✅ Safe | ✅ Safe |
| `gpio_pin_toggle_dt` | ✅ Safe (fast GPIO) | ✅ Safe |

## ⚠️ Critical Rule: ISR Context in Callbacks

Timer expiry and stop callbacks run in the **system timer interrupt context**. You must not sleep, block, or call functions that internally take mutexes (like `LOG_INF`). The safe pattern for logging from a callback is to submit a `k_work` item:

```c
static void periodic_expiry_fn(struct k_timer *timer)
{
    // ⚠️ ISR context
    atomic_inc(&fire_count);
    gpio_pin_toggle_dt(&led);    // fast, ISR-safe
    k_work_submit(&log_work);    // defer LOG_INF to thread context
}
```

## Expected Serial Output

```
========================================
  Conexio Stratus Pro — Timers
  Board: conexio_stratus_pro/nrf9151/ns
========================================

[00:00:00.001] <inf> timers: oneshot_timer started — will fire in 3000 ms
[00:00:00.001] <inf> timers: periodic_timer started — fires every 1000 ms
[00:00:00.001] <inf> timers: watchdog_timer started — fires every 5000 ms
[00:00:00.501] <inf> timers: oneshot_timer: ~2499 ms remaining until first expiry
[00:00:00.502] <inf> timers: periodic_timer: ~498 ms until next expiry
[00:00:00.502] <inf> timers: main: blocking on k_timer_status_sync(periodic_timer)...
[00:00:01.001] <inf> timers: log_work: periodic fired 1 times, watchdog fired 0 times
[00:00:01.002] <inf> timers: main: periodic_timer fired 1 time(s) while we waited
[00:00:02.001] <inf> timers: log_work: periodic fired 2 times, watchdog fired 0 times
[00:00:03.001] <inf> timers: log_work: periodic fired 3 times, watchdog fired 0 times
[TIMER CB] oneshot_timer expired at ~3001 ms uptime
[00:00:03.501] <inf> timers: periodic_timer fired 2 time(s) in the last ~2.5 s ...
[00:00:03.502] <inf> timers: main: stopping watchdog_timer early...
[TIMER CB] watchdog_timer was stopped early
[00:00:06.502] <inf> timers: --- loop #1 ---
[00:00:06.502] <inf> timers:   oneshot_timer:  remaining=0 ms (0 = stopped/expired)
[00:00:06.503] <inf> timers:   periodic_timer: remaining=499 ms, total fires=6
...
```

## Build

```bash
west build -b conexio_stratus_pro/nrf9151/ns -- \
  -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

## Flash

```bash
newtmgr -c serial image upload build/timers/zephyr/zephyr.signed.bin
```

## File Structure

```
timers/
├── CMakeLists.txt
├── prj.conf
├── sysbuild.conf
├── README.md
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay
└── src/
    └── main.c
```
