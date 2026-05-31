# RTOS Threads Sample — Conexio Stratus Pro

**Author:** Rajeev Piyare &lt;rajeev@conexiotech.com&gt;  
**Copyright:** Copyright (c) 2026 Conexio Technologies, Inc.  
**License:** Apache-2.0

## Overview

This sample is a thorough educational tour of Zephyr RTOS threading primitives running on the Conexio Stratus Pro (nRF9151). It deliberately uses multiple synchronisation and communication mechanisms in a single, well-commented application so you can see how they interact.

## RTOS Primitives Demonstrated

### 1. `K_THREAD_DEFINE` — Static thread creation

Threads defined at compile time with a single macro. The kernel allocates the stack and TCB in `.bss` and starts the thread automatically when the kernel boots. No explicit `k_thread_start()` required.

```c
K_THREAD_DEFINE(sensor_thread_id,
                SENSOR_STACK_SIZE,   // 1024 bytes
                sensor_thread_fn,
                NULL, NULL, NULL,
                SENSOR_PRIORITY,     // 5
                0, 0);
```

Two static threads: `sensor_thread` (priority 5) and `consumer_thread` (priority 6).

### 2. `k_thread_create` — Dynamic thread creation

Called at runtime from `main()`. Requires a pre-allocated stack (`K_THREAD_STACK_DEFINE`) and a `struct k_thread` (TCB) declared separately. Returns a `k_tid_t` you can use for `k_thread_suspend`, `k_thread_resume`, or `k_thread_abort`.

```c
heartbeat_tid = k_thread_create(
    &heartbeat_thread_data,
    heartbeat_stack_area,
    K_THREAD_STACK_SIZEOF(heartbeat_stack_area),
    heartbeat_thread_fn,
    NULL, NULL, NULL,
    HEARTBEAT_PRIORITY, 0, K_NO_WAIT);
```

The `heartbeat_thread` (priority 7) blinks LED0 every 1000 ms.

### 3. Message Queue (`K_MSGQ_DEFINE` / `k_msgq_put` / `k_msgq_get`)

A typed, fixed-depth FIFO for passing `struct sensor_data` between threads. Thread-safe by design — no extra locking required for the queue operations themselves.

```c
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), 8, 4);

// Producer (sensor_thread):
k_msgq_put(&sensor_msgq, &reading, K_NO_WAIT);

// Consumer (consumer_thread):
k_msgq_get(&sensor_msgq, &received, K_FOREVER);
```

`K_FOREVER` in `k_msgq_get` blocks the consumer until an item is available — no busy-poll.

### 4. Mutex (`k_mutex`)

Protects `shared_last_reading` from concurrent reads and writes. Any thread accessing the shared variable must hold `data_mutex`.

```c
K_MUTEX_DEFINE(data_mutex);

k_mutex_lock(&data_mutex, K_FOREVER);
shared_last_reading = reading;  // or read it
k_mutex_unlock(&data_mutex);
```

**Rule of thumb:** hold the mutex for the shortest time possible. Never sleep while holding a mutex.

### 5. Semaphore (`k_sem`)

Used as a binary "data ready" signal. `sensor_thread` calls `k_sem_give()` after each new measurement; other threads can call `k_sem_take()` to wait efficiently.

```c
K_SEM_DEFINE(data_ready_sem, 0, 1);  // count=0, max=1 (binary)

// Signal:
k_sem_give(&data_ready_sem);

// Wait (with timeout):
if (k_sem_take(&data_ready_sem, K_MSEC(200)) == 0) { ... }
```

## Thread Lifecycle

```
Kernel boot
  │
  ├─► sensor_thread  (static, auto-start)   prio 5
  ├─► consumer_thread (static, auto-start)  prio 6
  │
main() runs
  │
  └─► heartbeat_thread (dynamic, K_NO_WAIT) prio 7
        │
      main() returns — kernel keeps running all threads
```

Lower priority number = higher scheduling priority in Zephyr.

## Data Flow

```
sensor_thread
  ├── k_msgq_put(&sensor_msgq, &reading)
  ├── k_mutex_lock → update shared_last_reading → k_mutex_unlock
  └── k_sem_give(&data_ready_sem)
         │
         ▼
consumer_thread
  ├── k_msgq_get(&sensor_msgq, &received, K_FOREVER)
  ├── k_mutex_lock → read shared_last_reading → k_mutex_unlock
  └── k_sem_take(&data_ready_sem, K_MSEC(200))
```

## Expected Serial Output

```
========================================
  Conexio Stratus Pro — RTOS Threads
  Board: conexio_stratus_pro/nrf9151/ns
========================================

[00:00:00.001] <inf> rtos_threads: main: static threads already scheduled by kernel
[00:00:00.001] <inf> rtos_threads: main: sensor_thread tid=0x...
[00:00:00.001] <inf> rtos_threads: main: consumer_thread tid=0x...
[00:00:00.002] <inf> rtos_threads: sensor_thread started (tid=0x...)
[00:00:00.002] <inf> rtos_threads: consumer_thread started (tid=0x...)
[00:00:00.003] <inf> rtos_threads: main: heartbeat_thread created tid=0x...
[00:00:00.003] <inf> rtos_threads: main: msgq capacity=8, used=0
[00:00:00.003] <inf> rtos_threads: main: returning — threads continue running
[00:00:00.003] <inf> rtos_threads: heartbeat_thread started (tid=0x...)
[00:00:00.502] <inf> rtos_threads: consumer_thread: rcvd val=42 source=1 ts=502 ms
[00:00:01.002] <inf> rtos_threads: consumer_thread: rcvd val=59 source=1 ts=1002 ms
...
```

The LED blinks every 1000 ms driven by `heartbeat_thread`.

## Build

```bash
west build -b conexio_stratus_pro/nrf9151/ns -- \
  -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

## Flash

```bash
newtmgr -c serial image upload build/rtos_threads/zephyr/zephyr.signed.bin
```

## File Structure

```
rtos_threads/
├── CMakeLists.txt
├── prj.conf
├── sysbuild.conf
├── README.md
├── boards/
│   └── conexio_stratus_pro_nrf9151_ns.overlay
└── src/
    └── main.c
```
