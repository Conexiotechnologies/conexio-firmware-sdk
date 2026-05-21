# Group 1 — RTOS Foundations

A set of three educational samples that cover the core Zephyr RTOS building
blocks every nRF Connect SDK developer needs to understand before building
real IoT applications.

Each sample is self-contained, heavily commented, and designed to be used as
a copy-paste template. Work through them in order — threads first, then work
queues, then timers.

---

## Samples at a Glance

| Sample | Folder | Build output |
|--------|--------|--------------|
| RTOS Threads | `rtos_threads/` | `build/rtos_threads/zephyr/zephyr.signed.bin` |
| Work Queue | `work_queue/` | `build/work_queue/zephyr/zephyr.signed.bin` |
| Timers | `timers/` | `build/timers/zephyr/zephyr.signed.bin` |

Build command for all three (swap folder name):

```bash
west build -b conexio_stratus_pro/nrf9151/ns \
    samples/conexio_stratus/<folder> \
    -- -DBOARD_ROOT=/opt/nordic/ncs/v2.9.0/conexio-firmware-sdk
```

Flash:

```bash
newtmgr -c serial image upload build/<project>/zephyr/zephyr.signed.bin
```

---

## Sample 1 — RTOS Threads (`rtos_threads`)

### What it does

Three threads run concurrently. A **sensor thread** simulates a sensor reading
every 500 ms and pushes data into a message queue. A **consumer thread** drains
the queue and logs each reading. A **heartbeat thread** blinks the LED
independently every 1 second. All three communicate using the five primitives
below.

### Thread topology

```
sensor_thread (prio 5, static)
  │
  ├── k_msgq_put ──────────────► sensor_msgq ──► consumer_thread (prio 6, static)
  ├── k_mutex_lock/unlock ─────► shared_last_reading
  └── k_sem_give ──────────────► data_ready_sem

heartbeat_thread (prio 7, dynamic) — blinks LED0 independently
```

### Key concepts covered

#### `K_THREAD_DEFINE` — Static thread creation

Creates a thread at compile time. The kernel allocates the stack and starts the
thread automatically on boot — no explicit `k_thread_start()` required.

```c
K_THREAD_DEFINE(sensor_thread_id,
                1024,               /* stack size in bytes */
                sensor_thread_fn,   /* entry function      */
                NULL, NULL, NULL,   /* p1, p2, p3          */
                5,                  /* priority            */
                0, 0);              /* options, delay      */
```

#### `k_thread_create` — Dynamic thread creation

Creates a thread at runtime from `main()`. Requires a pre-allocated stack
(`K_THREAD_STACK_DEFINE`) and a `struct k_thread` declared separately. Returns
a `k_tid_t` for later `k_thread_suspend` / `k_thread_resume` / `k_thread_abort`.

```c
K_THREAD_STACK_DEFINE(heartbeat_stack, 512);
static struct k_thread heartbeat_tcb;

k_tid_t tid = k_thread_create(&heartbeat_tcb,
                               heartbeat_stack,
                               K_THREAD_STACK_SIZEOF(heartbeat_stack),
                               heartbeat_thread_fn,
                               NULL, NULL, NULL,
                               7, 0, K_NO_WAIT);
k_thread_name_set(tid, "heartbeat");
```

#### Message Queue — `K_MSGQ_DEFINE` / `k_msgq_put` / `k_msgq_get`

A typed, fixed-depth FIFO for inter-thread data transfer. Thread-safe by
design — no extra mutex needed for the queue operations.

```c
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 8, 4);

// Producer — K_NO_WAIT: return immediately if full
k_msgq_put(&sensor_msgq, &reading, K_NO_WAIT);

// Consumer — K_FOREVER: block until an item arrives (no busy-poll)
k_msgq_get(&sensor_msgq, &received, K_FOREVER);
```

#### Mutex — `K_MUTEX_DEFINE` / `k_mutex_lock` / `k_mutex_unlock`

Protects a shared variable from concurrent access. Always unlock after every
lock path, and never sleep while holding a mutex.

```c
K_MUTEX_DEFINE(data_mutex);

k_mutex_lock(&data_mutex, K_FOREVER);
shared_last_reading = reading;   /* critical section */
k_mutex_unlock(&data_mutex);
```

#### Semaphore — `K_SEM_DEFINE` / `k_sem_give` / `k_sem_take`

Used as a binary "data ready" signal. Initial count 0 means no data is
available at boot. `k_sem_give` from sensor thread unblocks any waiting thread.

```c
K_SEM_DEFINE(data_ready_sem, 0, 1);   /* initial=0, max=1 (binary) */

k_sem_give(&data_ready_sem);           /* signal from producer      */
k_sem_take(&data_ready_sem, K_MSEC(200)); /* wait with 200 ms timeout */
```

#### Priority rules

Lower number = higher scheduling priority in Zephyr. `sensor_thread` (5) will
preempt `consumer_thread` (6) which will preempt `heartbeat_thread` (7).
`main()` runs at priority 0 (highest) during startup, then returns.

---

## Sample 2 — Work Queue (`work_queue`)

### What it does

Demonstrates the most important Zephyr pattern for embedded firmware: getting
out of interrupt context safely. A GPIO button interrupt submits a work item
to the system work queue. A delayable work item on a custom work queue fires
every 2 seconds and chains to another work item. The sample shows when to use
each work queue type and how to guard against double-submission.

### Work item flow

```
Button press (ISR)
  │  k_work_submit(&sensor_work)  ← ISR-safe
  ▼
sensor_work_fn()  [system work queue — thread context]
  └── LOG_INF, k_msleep, gpio toggle — all safe here

periodic_dwork_fn()  [custom work queue, every 2 s]
  └── k_work_submit(&result_work)  → result_work_fn() [system work queue]
```

### Key concepts covered

#### System work queue vs custom work queue

```c
/* System work queue — always available, no setup */
k_work_submit(&my_work);

/* Custom work queue — configurable priority and stack */
K_THREAD_STACK_DEFINE(custom_wq_stack, 1024);
static struct k_work_q custom_work_q;

k_work_queue_start(&custom_work_q, custom_wq_stack,
                   K_THREAD_STACK_SIZEOF(custom_wq_stack),
                   5,     /* priority */
                   NULL); /* default config */

k_work_submit_to_queue(&custom_work_q, &my_work);
```

Use a custom work queue when you need a different priority from the system work
queue, a larger stack, or to isolate slow work from system-critical operations.

#### `k_work` — One-shot work item

```c
static void sensor_work_fn(struct k_work *work) {
    /* Thread context — safe to LOG, sleep, access peripherals */
    LOG_INF("sensor read triggered by button");
}
static K_WORK_DEFINE(sensor_work, sensor_work_fn);

k_work_submit(&sensor_work);   /* submit to system work queue */
```

#### `k_work_delayable` — Delayed / periodic work

Re-schedule from inside the handler to create a periodic pattern.

```c
static void periodic_fn(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    /* ... do work ... */
    k_work_reschedule_for_queue(&custom_work_q, dwork, K_MSEC(2000));
}
static K_WORK_DELAYABLE_DEFINE(periodic_dwork, periodic_fn);

/* First kick */
k_work_reschedule_for_queue(&custom_work_q, &periodic_dwork, K_MSEC(2000));
```

#### ISR → work queue — the canonical pattern

> **Rule:** Do only the minimum inside an ISR. Submit a work item for
> everything else.

```c
static void button_isr(const struct device *dev,
                       struct gpio_callback *cb, uint32_t pins)
{
    /* ISR context — LOG_INF is NOT safe, printk IS safe */
    printk("[ISR] button pressed\n");
    k_work_submit(&sensor_work);   /* ISR-safe */
}
```

What is and isn't safe in ISR context:

| Operation | ISR-safe? |
|-----------|-----------|
| `printk()` | ✅ Yes |
| `k_work_submit()` | ✅ Yes |
| `atomic_inc()` | ✅ Yes |
| `k_sem_give()` | ✅ Yes |
| `LOG_INF()` | ❌ No — may take a mutex |
| `k_msleep()` | ❌ No — cannot block |
| `k_mutex_lock()` | ❌ No — may block |

#### `k_work_is_pending()` guard

```c
if (!k_work_is_pending(&result_work)) {
    k_work_submit(&result_work);
} else {
    LOG_WRN("still running/queued, skipping");
}
```

---

## Sample 3 — Timers (`timers`)

### What it does

Three timers run simultaneously. A **one-shot timer** fires after 3 seconds.
A **periodic timer** fires every second and blinks the LED — demonstrating the
correct ISR-safe GPIO toggle pattern. A **watchdog-style timer** fires every 5
seconds and has both an expiry callback and a stop callback. The sample shows
every polling and blocking pattern available for timers.

### Key concepts covered

#### `K_TIMER_DEFINE` — Static timer declaration

```c
K_TIMER_DEFINE(my_timer, expiry_fn, stop_fn);
//                        ^^^^^^^^   ^^^^^^^
//                     on expiry   on k_timer_stop() (NULL if not needed)
```

#### One-shot timer

```c
/* Fires once after 3 s, then stops */
k_timer_start(&oneshot_timer, K_MSEC(3000), K_NO_WAIT);
```

Re-arm with another `k_timer_start` after it expires.

#### Periodic timer

```c
/* First fire after 1 s, then every 1 s */
k_timer_start(&periodic_timer, K_MSEC(1000), K_MSEC(1000));
```

#### Stop callback

The stop callback fires when `k_timer_stop()` is called on a running timer.
It does **not** fire on normal expiry.

```c
K_TIMER_DEFINE(watchdog_timer, expiry_fn, stop_fn);
k_timer_stop(&watchdog_timer);   /* → stop_fn() called synchronously */
```

#### `k_timer_remaining_get()` — Time until next expiry

```c
uint32_t ms = k_timer_remaining_get(&oneshot_timer);
/* Returns 0 if stopped or already expired */
```

#### `k_timer_status_get()` — Non-blocking expiry count

Returns and **resets** the expiry counter. Safe from any context.

```c
uint32_t count = k_timer_status_get(&periodic_timer);
/* count = times fired since last call; counter is now 0 */
```

#### `k_timer_status_sync()` — Blocking thread wait

Blocks until the timer fires at least once. The cleanest way to sync a thread
to a periodic timer without busy-polling or a separate semaphore.

```c
uint32_t n = k_timer_status_sync(&periodic_timer);
/* n = number of expirations that occurred while blocking */
/* Only valid from thread context — NOT from an ISR */
```

#### ⚠️ Critical rule: callback context

Timer callbacks run in the **system timer ISR**. You cannot sleep or log.

```c
static void periodic_expiry_fn(struct k_timer *timer)
{
    /* ⚠️ ISR context */
    atomic_inc(&fire_count);
    gpio_pin_toggle_dt(&led);    /* fast GPIO — ISR-safe           */
    k_work_submit(&log_work);    /* defer LOG_INF to thread context */
}
```

| In a timer callback | Safe? |
|---------------------|-------|
| `printk()` | ✅ Yes |
| `k_work_submit()` | ✅ Yes |
| `atomic_inc()` | ✅ Yes |
| `gpio_pin_toggle_dt()` | ✅ Yes (fast GPIO) |
| `LOG_INF()` | ❌ No |
| `k_msleep()` | ❌ No |
| `k_timer_status_sync()` | ❌ No (thread-only) |

---

## Quick Reference

### Choosing between a timer and a work queue

| Need | Use |
|------|-----|
| Periodic action at a fixed interval | `k_timer` + periodic expiry |
| One-shot delay | `k_timer` one-shot, or `k_work_delayable` |
| Deferred work from an ISR | `k_work` submitted from ISR |
| Periodic action that needs to log, sleep, or access peripherals | `k_work_delayable` reschedule pattern |
| Complex periodic logic (long-running) | `k_work_delayable` on a custom work queue |

### Choosing between a mutex, semaphore, and message queue

| Need | Use |
|------|-----|
| Protect a shared variable | `k_mutex` |
| Signal that an event occurred (binary) | `k_sem` (count=0, max=1) |
| Rate-limit access to N resources | `k_sem` (count=N, max=N) |
| Pass data between threads | `k_msgq` |
| Pass large or variable-size data | `k_fifo` with heap-allocated nodes |

### Thread priority cheat sheet

```
Priority 0  ← main() startup thread (highest)
Priority 1-4  ← high-priority drivers, time-critical work
Priority 5-7  ← application threads (used in rtos_threads sample)
Priority 14   ← system work queue default
Priority 15+  ← background/idle tasks
```

Lower number = higher priority. Threads at the same priority are
round-robin scheduled.

---

## Further Reading

- [Zephyr Kernel Primer](https://docs.zephyrproject.org/latest/kernel/index.html)
- [nRF Connect SDK — Thread documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/kernel/services/threads/index.html)
- Individual sample READMEs:
  - [`rtos_threads/README.md`](rtos_threads/README.md)
  - [`work_queue/README.md`](work_queue/README.md)
  - [`timers/README.md`](timers/README.md)
