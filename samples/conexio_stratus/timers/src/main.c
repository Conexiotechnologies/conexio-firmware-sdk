/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr Timers Educational Sample
 * ==================================
 * Zephyr kernel timers fire callbacks and/or unblock threads waiting on
 * them.  This sample demonstrates:
 *
 *  1. K_TIMER_DEFINE          — static timer declaration
 *  2. One-shot timer          — fires once after a delay
 *  3. Periodic timer          — fires repeatedly at a fixed interval
 *  4. Timer with stop callback — both expiry AND stop callbacks
 *  5. k_timer_status_get()    — how many times the timer has fired
 *  6. k_timer_remaining_get() — milliseconds until next expiry
 *  7. k_timer_stop()          — cancel a running timer
 *  8. Callback context rules  — what is/isn't allowed in callbacks
 *  9. LED blink from timer    — hardware effect driven by a timer
 * 10. k_timer_status_sync()   — blocking wait in thread context
 *
 * ⚠️  CRITICAL RULE — Timer expiry callbacks run in INTERRUPT context
 * (the system timer ISR).  Inside a callback you MUST NOT:
 *   - Call k_sleep() / k_msleep()
 *   - Call LOG_INF() (takes a mutex internally)
 *   - Block on a semaphore, mutex, or message queue with a timeout
 *   - Access slow peripherals synchronously
 *
 * You CAN safely do in a callback:
 *   - printk()                  (interrupt-safe)
 *   - k_work_submit()           (interrupt-safe, defers work to a thread)
 *   - atomic_inc() / atomic_get()
 *   - gpio_pin_toggle_dt()      (fast GPIO is typically IRQ-safe)
 *   - k_sem_give()              (interrupt-safe)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(timers, LOG_LEVEL_DBG);

/* =========================================================================
 * Hardware — LED (blinked by the periodic timer callback)
 * =========================================================================
 */
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

/* =========================================================================
 * Shared atomic counters — safe to use from callback (ISR) context
 * =========================================================================
 */
static atomic_t periodic_fire_count;  /* incremented in periodic callback */
static atomic_t watchdog_fire_count;  /* incremented in watchdog callback */

/* =========================================================================
 * Work item — used to log from timer callbacks safely
 *
 * Because LOG_INF is NOT ISR-safe we use a k_work item.
 * The timer callback submits it to the system work queue; the actual
 * LOG_INF runs in thread context.
 * =========================================================================
 */
static void log_work_fn(struct k_work *work);
static K_WORK_DEFINE(log_work, log_work_fn);

static void log_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	LOG_INF("log_work: periodic fired %d times, watchdog fired %d times",
		(int)atomic_get(&periodic_fire_count),
		(int)atomic_get(&watchdog_fire_count));
}

/* =========================================================================
 * TIMER 1 — One-shot timer
 *
 * k_timer_start(timer, duration, period)
 *   duration = K_MSEC(3000) : fires once after 3 seconds
 *   period   = K_NO_WAIT    : do not repeat
 *
 * After the single expiry the timer becomes idle.
 * =========================================================================
 */
static void oneshot_expiry_fn(struct k_timer *timer)
{
	/*
	 * ⚠️  INTERRUPT CONTEXT — no sleep, no LOG_INF.
	 * printk() is safe here.
	 */
	printk("[TIMER CB] oneshot_timer expired at ~%u ms uptime\n",
	       k_uptime_get_32());
}

K_TIMER_DEFINE(oneshot_timer, oneshot_expiry_fn, NULL);

/* =========================================================================
 * TIMER 2 — Periodic timer
 *
 * k_timer_start(timer, duration, period)
 *   duration = K_MSEC(1000) : first fire after 1 second
 *   period   = K_MSEC(1000) : then every 1 second
 *
 * The LED is toggled on each expiry.  Because gpio_pin_toggle_dt is
 * fast and GPIO drivers on nRF are IRQ-safe, this is acceptable.
 * For complex GPIO sequences, use k_work_submit instead.
 * =========================================================================
 */
static void periodic_expiry_fn(struct k_timer *timer)
{
	/* ⚠️  INTERRUPT CONTEXT */
	atomic_inc(&periodic_fire_count);

	/* Blink the LED — fast, ISR-safe GPIO toggle */
	gpio_pin_toggle_dt(&led);

	/* Defer logging to thread context via work queue */
	k_work_submit(&log_work);
}

K_TIMER_DEFINE(periodic_timer, periodic_expiry_fn, NULL);

/* =========================================================================
 * TIMER 3 — Watchdog-style timer with BOTH expiry and stop callbacks
 *
 * The stop callback fires when k_timer_stop() is called while the
 * timer is running.  Useful for cleanup, state machine transitions,
 * or logging that the timer was cancelled.
 *
 * k_timer_start(timer, K_MSEC(5000), K_MSEC(5000)) — 5 s periodic
 * =========================================================================
 */
static void watchdog_expiry_fn(struct k_timer *timer)
{
	/* ⚠️  INTERRUPT CONTEXT */
	atomic_inc(&watchdog_fire_count);
	printk("[TIMER CB] watchdog_timer expired (fire #%d)\n",
	       (int)atomic_get(&watchdog_fire_count));
}

static void watchdog_stop_fn(struct k_timer *timer)
{
	/*
	 * ⚠️  INTERRUPT CONTEXT — called when k_timer_stop() is invoked
	 * on a running timer.  NOT called if the timer expires normally.
	 */
	printk("[TIMER CB] watchdog_timer was stopped early\n");
}

K_TIMER_DEFINE(watchdog_timer, watchdog_expiry_fn, watchdog_stop_fn);

/* =========================================================================
 * main() — start timers, then demonstrate polling patterns
 * =========================================================================
 */
int main(void)
{
	printk("\n========================================\n");
	printk("  Conexio Stratus Pro — Timers\n");
	printk("  Board: %s\n", CONFIG_BOARD);
	printk("========================================\n\n");

	/* --- LED setup --- */
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED GPIO not ready");
		return -ENODEV;
	}
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	/* -----------------------------------------------------------------------
	 * Start TIMER 1 — one-shot, fires after 3 s
	 *
	 * k_timer_start(timer, duration, period)
	 *   duration: delay before first expiry
	 *   period  : interval between subsequent expiries (K_NO_WAIT = no repeat)
	 * -----------------------------------------------------------------------
	 */
	k_timer_start(&oneshot_timer, K_MSEC(3000), K_NO_WAIT);
	LOG_INF("oneshot_timer started — will fire in 3000 ms");

	/* -----------------------------------------------------------------------
	 * Start TIMER 2 — periodic, fires every 1 s, LED blinks
	 * -----------------------------------------------------------------------
	 */
	k_timer_start(&periodic_timer, K_MSEC(1000), K_MSEC(1000));
	LOG_INF("periodic_timer started — fires every 1000 ms");

	/* -----------------------------------------------------------------------
	 * Start TIMER 3 — watchdog-style, 5 s periodic
	 * -----------------------------------------------------------------------
	 */
	k_timer_start(&watchdog_timer, K_MSEC(5000), K_MSEC(5000));
	LOG_INF("watchdog_timer started — fires every 5000 ms");

	/* -----------------------------------------------------------------------
	 * POLLING PATTERN 1 — k_timer_remaining_get()
	 *
	 * Returns milliseconds until the next expiry of the timer.
	 * Returns 0 if the timer is stopped or has already expired.
	 * Safe to call from any context.
	 * -----------------------------------------------------------------------
	 */
	k_msleep(500); /* Wait 500 ms so the remaining time is meaningful */
	uint32_t remaining = k_timer_remaining_get(&oneshot_timer);
	LOG_INF("oneshot_timer: ~%u ms remaining until first expiry", remaining);

	remaining = k_timer_remaining_get(&periodic_timer);
	LOG_INF("periodic_timer: ~%u ms until next expiry", remaining);

	/* -----------------------------------------------------------------------
	 * BLOCKING WAIT PATTERN — k_timer_status_sync()
	 *
	 * Blocks the calling thread until the timer fires at least once
	 * since the last call to k_timer_status_sync() or k_timer_start().
	 * Returns the number of expirations that occurred.
	 *
	 * This is the cleanest way to synchronise a thread to a periodic
	 * timer without busy-polling or using a separate semaphore.
	 * -----------------------------------------------------------------------
	 */
	LOG_INF("main: blocking on k_timer_status_sync(periodic_timer)...");

	uint32_t expirations = k_timer_status_sync(&periodic_timer);

	LOG_INF("main: periodic_timer fired %u time(s) while we waited",
		expirations);

	/* -----------------------------------------------------------------------
	 * POLLING PATTERN 2 — k_timer_status_get()
	 *
	 * Returns the number of times the timer has fired since the last
	 * call to k_timer_status_get() or k_timer_start().  Calling this
	 * function resets the internal counter to 0 (it is consumed, not
	 * peeked).  Non-blocking — safe from any context.
	 * -----------------------------------------------------------------------
	 */
	k_msleep(2500); /* Let the periodic timer fire a couple more times */

	uint32_t status = k_timer_status_get(&periodic_timer);
	LOG_INF("periodic_timer fired %u time(s) in the last ~2.5 s "
		"(status consumed)", status);

	/* -----------------------------------------------------------------------
	 * k_timer_stop() — stop the watchdog timer early to demonstrate
	 * the stop callback firing.
	 * -----------------------------------------------------------------------
	 */
	LOG_INF("main: stopping watchdog_timer early...");
	k_timer_stop(&watchdog_timer);
	/* watchdog_stop_fn() will have been called by now (synchronously) */

	/* -----------------------------------------------------------------------
	 * Main loop — polls remaining time and status every 3 seconds.
	 *
	 * After the one-shot fires (at ~3 s) k_timer_remaining_get returns 0.
	 * The periodic_timer continues running indefinitely.
	 * -----------------------------------------------------------------------
	 */
	uint32_t loop_count = 0;

	while (1) {
		k_msleep(3000);
		loop_count++;

		uint32_t oneshot_rem = k_timer_remaining_get(&oneshot_timer);
		uint32_t periodic_rem = k_timer_remaining_get(&periodic_timer);

		LOG_INF("--- loop #%u ---", loop_count);
		LOG_INF("  oneshot_timer:  remaining=%u ms (0 = stopped/expired)",
			oneshot_rem);
		LOG_INF("  periodic_timer: remaining=%u ms, total fires=%d",
			periodic_rem, (int)atomic_get(&periodic_fire_count));
		LOG_INF("  watchdog_timer: total fires=%d (stopped at loop 0)",
			(int)atomic_get(&watchdog_fire_count));

		/*
		 * Demonstrate re-starting the one-shot timer after it has expired.
		 * Re-arm it for another 3 s on even loops.
		 */
		if (loop_count % 2 == 0) {
			LOG_INF("  Re-arming oneshot_timer for 3000 ms");
			k_timer_start(&oneshot_timer, K_MSEC(3000), K_NO_WAIT);
		}
	}

	return 0;
}
