# Work Queue Sample — Conexio Stratus Pro

**Author:** Rajeev Piyare &lt;rajeev@conexiotech.com&gt;  
**Copyright:** Copyright (c) 2026 Conexio Technologies, Inc.  
**License:** Apache-2.0

## Overview

This sample provides an educational tour of Zephyr work queues on the Conexio Stratus Pro (nRF9151). It covers regular work items, delayable work, the critical ISR→work queue pattern, work chaining, and the difference between the system work queue and a custom work queue.

## Work Queue Primitives Demonstrated

### 1. System Work Queue vs Custom Work Queue

| | System Work Queue | Custom Work Queue |
|---|---|---|
| API | `k_work_submit(work)` | `k_work_submit_to_queue(&my_wq, work)` |
| Priority | `CONFIG_SYSTEM_WORKQUEUE_PRIORITY` | Configurable |
| Stack size | `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` | Configurable |
| Setup | None — always available | `k_work_queue_start()` |

The system work queue is convenient and always ready. Use a custom work queue when you need a different priority, a larger stack, or to isolate potentially slow work from other system work.

```c
// Custom work queue setup:
K_THREAD_STACK_DEFINE(custom_wq_stack, 1024);
static struct k_work_q custom_work_q;

k_work_queue_start(&custom_work_q, custom_wq_stack,
                   K_THREAD_STACK_SIZEOF(custom_wq_stack),
                   5,    // priority
                   NULL);
```

### 2. `k_work` — One-shot work item

The simplest work item. Submit it; it runs once; then it is idle again.

```c
static void sensor_work_fn(struct k_work *work) {
    // Thread context — safe to log, sleep, etc.
    LOG_INF("Processing sensor reading...");
}
static K_WORK_DEFINE(sensor_work, sensor_work_fn);

// Submit to system work queue:
k_work_submit(&sensor_work);

// Submit to custom work queue:
k_work_submit_to_queue(&custom_work_q, &sensor_work);
```

### 3. `k_work_delayable` — Delayed/periodic work

Wraps `k_work` with a countdown timer. To make it periodic, reschedule from inside its own handler.

```c
static void periodic_dwork_fn(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    // ... do work ...
    // Reschedule for next period:
    k_work_reschedule_for_queue(&custom_work_q, dwork, K_MSEC(2000));
}
static K_WORK_DELAYABLE_DEFINE(periodic_dwork, periodic_dwork_fn);

// First kick:
k_work_reschedule_for_queue(&custom_work_q, &periodic_dwork, K_MSEC(2000));
```

### 4. ISR → Work Queue Pattern

**The most important work queue pattern.** GPIO (and other hardware) interrupts run in interrupt context where you **cannot** sleep, log, or call most kernel APIs. The solution: do only the minimum in the ISR, then submit a work item to handle the rest in thread context.

```
Button press
    │
    ▼  (interrupt context — NO sleep, NO LOG_INF)
button_isr()
    │  k_work_submit(&sensor_work)   ← ISR-safe
    │
    ▼  (system work queue thread — full thread context)
sensor_work_fn()
    │  LOG_INF(...)  ← safe here
    │  k_msleep(10)  ← safe here
    └─ gpio_pin_toggle_dt(...)
```

```c
static void button_isr(const struct device *dev,
                       struct gpio_callback *cb, uint32_t pins)
{
    printk("[ISR] button pressed\n");  // printk is ISR-safe
    k_work_submit(&sensor_work);       // defer real work to thread context
}
```

### 5. Work Chaining

`periodic_dwork_fn` submits `result_work` to the system work queue after it finishes. This chains two work items on two different queues.

### 6. `k_work_is_pending()` Guard

Before submitting a work item that might still be running or queued, check first:

```c
if (!k_work_is_pending(&result_work)) {
    k_work_submit(&result_work);
} else {
    LOG_WRN("still pending, skipping");
}
```

## Button Wiring

The button uses `gpio0` pin 31 with `GPIO_PULL_UP | GPIO_ACTIVE_LOW`. The interrupt is configured as `GPIO_INT_EDGE_TO_ACTIVE` — triggers when the pin transitions to its active state (low for an active-low button).

## Expected Serial Output

```
========================================
  Conexio Stratus Pro — Work Queues
  Board: conexio_stratus_pro/nrf9151/ns
========================================

[00:00:00.001] <inf> work_queue: Button on pin 31 configured with edge interrupt
[00:00:00.001] <inf> work_queue: Custom work queue started at priority 5
[00:00:00.001] <inf> work_queue: Periodic work scheduled every 2000 ms on custom wq
[00:00:02.001] <inf> work_queue: periodic_dwork: fired #1 (custom wq, every 2000 ms)
[00:00:02.001] <inf> work_queue: result_work: posting result to system wq (total processed=0)
[00:00:03.001] <inf> work_queue: main loop: periodic_dwork pending=yes, total_processed=0

--- (button press) ---
[ISR] button pressed — submitting sensor_work
[00:00:03.5xx] <inf> work_queue: sensor_work: processing button-triggered sensor read (count=0)

[00:00:04.001] <inf> work_queue: periodic_dwork: fired #2 ...
...
```

## Build

```bash
west build -b conexio_stratus_pro/nrf9151/ns -- \
  -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

## Flash

```bash
newtmgr -c serial image upload build/work_queue/zephyr/zephyr.signed.bin
```

## File Structure

```
work_queue/
├── CMakeLists.txt
├── prj.conf
├── sysbuild.conf
├── README.md
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay
└── src/
    └── main.c
```
