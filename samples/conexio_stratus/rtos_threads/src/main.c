/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr RTOS Threads Educational Sample
 * ========================================
 * This sample demonstrates the core Zephyr RTOS threading primitives:
 *
 *  1. K_THREAD_DEFINE    — static thread creation at compile time
 *  2. k_thread_create    — dynamic thread creation at runtime
 *  3. K_MSGQ_DEFINE      — message queue for inter-thread data passing
 *  4. k_mutex            — mutual exclusion for shared data
 *  5. k_sem              — semaphore used as a "data ready" signal
 *
 * Thread topology:
 *
 *   [sensor_thread]  -- pushes sensor_data --> [msgq]
 *                                                  |
 *                    -- signals ready_sem -->  [consumer_thread]
 *                                                  |
 *                                          reads shared_last_reading
 *                                          (protected by data_mutex)
 *
 *   [heartbeat_thread] -- blinks LED0 independently (dynamic thread)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(rtos_threads, LOG_LEVEL_DBG);

/* =========================================================================
 * Hardware — LED (heartbeat_thread blinks this)
 * =========================================================================
 */
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

/* =========================================================================
 * Sensor data structure passed through the message queue
 * =========================================================================
 */
struct sensor_data {
	uint32_t timestamp_ms; /* k_uptime_get_32() at time of reading  */
	int32_t  value;        /* Simulated sensor value                 */
	uint8_t  source;       /* Sensor ID / source identifier          */
};

/* =========================================================================
 * PRIMITIVE 1 — Message Queue (k_msgq)
 *
 * K_MSGQ_DEFINE(name, msg_size, max_msgs, align)
 *
 * Creates a statically allocated message queue that holds up to 8 items
 * of type struct sensor_data.  Producers call k_msgq_put(); consumers
 * call k_msgq_get().  The queue is thread-safe by design — no extra
 * mutex needed for the queue itself.
 * =========================================================================
 */
#define MSGQ_DEPTH 8
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_data), MSGQ_DEPTH, 4);

/* =========================================================================
 * PRIMITIVE 2 — Mutex (k_mutex)
 *
 * Protects shared_last_reading from concurrent access.
 * Any thread that reads or writes shared_last_reading must hold
 * data_mutex.  k_mutex_lock() / k_mutex_unlock() form the critical
 * section.
 * =========================================================================
 */
static K_MUTEX_DEFINE(data_mutex);
static struct sensor_data shared_last_reading; /* Protected by data_mutex */

/* =========================================================================
 * PRIMITIVE 3 — Semaphore (k_sem)
 *
 * Used as a "data-ready" signal between sensor_thread and the main
 * thread (or any interested party).  The semaphore starts at 0; the
 * sensor thread gives it (k_sem_give) each time a new reading is
 * placed in the queue.  Another thread can k_sem_take with a timeout
 * to wait for fresh data without busy-polling.
 * =========================================================================
 */
static K_SEM_DEFINE(data_ready_sem,
		    0,  /* initial count — nobody has signalled yet */
		    1); /* max count — acts as a binary semaphore   */

/* =========================================================================
 * Thread stack and priority constants
 * =========================================================================
 */
#define SENSOR_STACK_SIZE    1024
#define CONSUMER_STACK_SIZE  1024
#define HEARTBEAT_STACK_SIZE 512

/* Lower number = higher priority in Zephyr preemptive scheduling */
#define SENSOR_PRIORITY    5
#define CONSUMER_PRIORITY  6
#define HEARTBEAT_PRIORITY 7

/* =========================================================================
 * PRIMITIVE 4a — Stack for the dynamic heartbeat thread
 *
 * K_THREAD_STACK_DEFINE reserves the memory region; k_thread_create
 * later assigns it to a thread.  Stacks for dynamic threads must be
 * declared with this macro (not plain arrays) so Zephyr can apply
 * correct alignment and MPU/stack-guard configuration.
 * =========================================================================
 */
K_THREAD_STACK_DEFINE(heartbeat_stack_area, HEARTBEAT_STACK_SIZE);
static struct k_thread heartbeat_thread_data; /* Thread control block (TCB) */

/* =========================================================================
 * SENSOR THREAD — simulates reading a sensor every 500 ms
 *
 * This thread is created STATICALLY with K_THREAD_DEFINE (see below
 * after the function body, which is the required pattern).
 *
 * What it does:
 *  1. Builds a sensor_data struct with a simulated value
 *  2. Pushes it to the message queue (non-blocking put with K_NO_WAIT;
 *     if full, the oldest item is discarded by k_msgq_purge)
 *  3. Updates the shared_last_reading under the mutex
 *  4. Signals data_ready_sem so watchers know fresh data arrived
 * =========================================================================
 */
static void sensor_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	uint32_t counter = 0;
	struct sensor_data reading;

	LOG_INF("sensor_thread started (tid=%p)", k_current_get());

	while (1) {
		/* --- Simulate a sensor measurement --- */
		reading.timestamp_ms = k_uptime_get_32();
		reading.value        = (int32_t)(counter * 17 + 42); /* fake data */
		reading.source       = 1U; /* sensor ID 1 */

		LOG_DBG("sensor_thread: new reading val=%d ts=%u ms",
			reading.value, reading.timestamp_ms);

		/*
		 * Push to queue.  k_msgq_put(queue, data, timeout)
		 * K_NO_WAIT means: return immediately if full rather than
		 * blocking.  We handle the full-queue case by purging.
		 */
		if (k_msgq_put(&sensor_msgq, &reading, K_NO_WAIT) != 0) {
			LOG_WRN("sensor_thread: queue full, purging oldest");
			k_msgq_purge(&sensor_msgq);
			/* Retry after purge */
			(void)k_msgq_put(&sensor_msgq, &reading, K_NO_WAIT);
		}

		/*
		 * Update the shared "last reading" variable.
		 * CRITICAL SECTION — acquire mutex before writing.
		 * k_mutex_lock(mutex, timeout): K_FOREVER = wait until free.
		 */
		k_mutex_lock(&data_mutex, K_FOREVER);
		shared_last_reading = reading;
		k_mutex_unlock(&data_mutex);

		/*
		 * Signal "data ready" semaphore.
		 * k_sem_give increments the semaphore count (up to max_count).
		 * Any thread blocked in k_sem_take will be unblocked.
		 */
		k_sem_give(&data_ready_sem);

		counter++;
		k_msleep(500); /* Sleep 500 ms between readings */
	}
}

/* =========================================================================
 * CONSUMER THREAD — waits on the message queue and logs received data
 *
 * Also statically created with K_THREAD_DEFINE.
 *
 * Demonstrates:
 *  - k_msgq_get with a timeout (blocks until an item arrives)
 *  - Reading shared_last_reading under the mutex
 *  - k_sem_take to optionally wait for a signal from sensor_thread
 * =========================================================================
 */
static void consumer_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct sensor_data received;

	LOG_INF("consumer_thread started (tid=%p)", k_current_get());

	while (1) {
		/*
		 * k_msgq_get(queue, buffer, timeout)
		 * K_FOREVER: block until an item is available.
		 * This is the preferred pattern — no CPU time wasted polling.
		 */
		if (k_msgq_get(&sensor_msgq, &received, K_FOREVER) == 0) {
			LOG_INF("consumer_thread: rcvd val=%d source=%u ts=%u ms",
				received.value, received.source,
				received.timestamp_ms);

			/*
			 * Also read the shared variable to show mutex usage.
			 * CRITICAL SECTION — acquire mutex before reading.
			 */
			k_mutex_lock(&data_mutex, K_FOREVER);
			LOG_DBG("consumer_thread: shared_last val=%d",
				shared_last_reading.value);
			k_mutex_unlock(&data_mutex);
		}

		/*
		 * Optionally wait for the semaphore (up to 200 ms).
		 * This demonstrates k_sem_take with a timeout — it either
		 * returns 0 if the semaphore was signalled, or -EAGAIN on
		 * timeout.  We don't block here with K_FOREVER so the
		 * consumer can keep draining the queue even without a signal.
		 */
		if (k_sem_take(&data_ready_sem, K_MSEC(200)) == 0) {
			LOG_DBG("consumer_thread: data_ready_sem taken");
		}
	}
}

/* =========================================================================
 * HEARTBEAT THREAD — blinks LED every 1000 ms
 *
 * Created DYNAMICALLY with k_thread_create() in main().
 * This shows the runtime thread creation path where you need the
 * thread handle (k_tid_t) for later operations like k_thread_abort().
 * =========================================================================
 */
static void heartbeat_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("heartbeat_thread started (tid=%p)", k_current_get());

	/* Configure LED pin as output */
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED GPIO device not ready");
		return;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	while (1) {
		gpio_pin_toggle_dt(&led);
		LOG_DBG("heartbeat_thread: LED toggled");
		k_msleep(1000);
	}
}

/* =========================================================================
 * PRIMITIVE 4b — Static thread creation with K_THREAD_DEFINE
 *
 * K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay)
 *
 *  name       — C identifier for the thread; also its k_tid_t
 *  stack_size — size in bytes (the macro allocates the stack too)
 *  entry      — thread entry function
 *  p1/p2/p3   — arguments passed to entry (NULL here)
 *  prio       — scheduling priority (lower = higher priority)
 *  options    — e.g. K_FP_REGS for floating-point; 0 = defaults
 *  delay      — start delay in ms; 0 = start immediately
 *
 * The threads below are created at build time and start automatically
 * when the kernel boots.  No k_thread_start() call is needed.
 * =========================================================================
 */
K_THREAD_DEFINE(sensor_thread_id,
		SENSOR_STACK_SIZE,
		sensor_thread_fn,
		NULL, NULL, NULL,
		SENSOR_PRIORITY, 0, 0);

K_THREAD_DEFINE(consumer_thread_id,
		CONSUMER_STACK_SIZE,
		consumer_thread_fn,
		NULL, NULL, NULL,
		CONSUMER_PRIORITY, 0, 0);

/* =========================================================================
 * main() — initialization and dynamic thread launch
 *
 * main() itself runs as a thread (the "main thread") at priority 0.
 * After setup it simply returns — the static threads above keep running
 * because the kernel scheduler continues as long as any thread exists.
 * =========================================================================
 */
int main(void)
{
	k_tid_t heartbeat_tid;

	printk("\n========================================\n");
	printk("  Conexio Stratus Pro — RTOS Threads\n");
	printk("  Board: %s\n", CONFIG_BOARD);
	printk("========================================\n\n");

	LOG_INF("main: static threads already scheduled by kernel");
	LOG_INF("main: sensor_thread tid=%p", (void *)sensor_thread_id);
	LOG_INF("main: consumer_thread tid=%p", (void *)consumer_thread_id);

	/*
	 * DYNAMIC THREAD CREATION — k_thread_create()
	 *
	 * k_thread_create(thread, stack, stack_size,
	 *                 entry, p1, p2, p3,
	 *                 prio, options, delay)
	 *
	 * Unlike K_THREAD_DEFINE the stack area (heartbeat_stack_area)
	 * and TCB (heartbeat_thread_data) were declared separately above.
	 * k_thread_create returns the k_tid_t which you can store and use
	 * to suspend, resume, or abort the thread later.
	 */
	heartbeat_tid = k_thread_create(
		&heartbeat_thread_data,       /* TCB storage          */
		heartbeat_stack_area,         /* Stack memory region  */
		K_THREAD_STACK_SIZEOF(heartbeat_stack_area),
		heartbeat_thread_fn,          /* Entry function       */
		NULL, NULL, NULL,             /* No arguments needed  */
		HEARTBEAT_PRIORITY,           /* Priority             */
		0,                            /* Options              */
		K_NO_WAIT);                   /* Start immediately    */

	/* Assign a human-readable name (visible in debugger / shell) */
	k_thread_name_set(heartbeat_tid, "heartbeat");

	LOG_INF("main: heartbeat_thread created tid=%p", (void *)heartbeat_tid);

	/*
	 * Print queue and semaphore state before returning.
	 * k_msgq_num_used_get() — items currently in the queue.
	 */
	LOG_INF("main: msgq capacity=%d, used=%d",
		MSGQ_DEPTH, k_msgq_num_used_get(&sensor_msgq));

	LOG_INF("main: returning — threads continue running");

	/*
	 * Returning from main() is fine in Zephyr.  The kernel keeps
	 * all spawned threads alive.  The main thread itself terminates
	 * but does not affect other threads.
	 */
	return 0;
}
