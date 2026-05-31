/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr Work Queue Educational Sample
 * ======================================
 * Work queues allow interrupt handlers (and other contexts where sleeping
 * is forbidden) to defer work to a thread context.  This sample covers:
 *
 *  1. System work queue  — the kernel's built-in work queue
 *  2. Custom work queue  — a user-defined work queue with its own thread
 *  3. k_work             — a one-shot work item
 *  4. k_work_delayable   — a work item that fires after a delay (periodic
 *                          pattern via k_work_reschedule_for_queue)
 *  5. ISR → work queue   — GPIO button interrupt submits a work item
 *  6. Work chaining      — delayable handler submits another work item
 *  7. k_work_is_pending  — guard against double-submission
 *
 * Work item flow:
 *
 *   Button ISR  ──submit──►  sensor_work  (system wq)
 *                                │
 *   periodic_dwork (custom wq) ──submit──►  result_work (system wq)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(work_queue, LOG_LEVEL_DBG);

/* =========================================================================
 * Hardware — button and LED
 * =========================================================================
 */
#define LED_NODE    DT_ALIAS(led0)
#define BUTTON_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec led    = GPIO_DT_SPEC_GET(LED_NODE,    gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);

/* GPIO callback structure (used by the button interrupt) */
static struct gpio_callback button_cb_data;

/* =========================================================================
 * CUSTOM WORK QUEUE
 *
 * A custom work queue gives you control over:
 *  - Thread priority (system wq runs at CONFIG_SYSTEM_WORKQUEUE_PRIORITY)
 *  - Stack size (tune for your actual work load)
 *  - Thread name (easier debugging)
 *
 * Steps:
 *  1. Allocate a stack with K_THREAD_STACK_DEFINE
 *  2. Declare a struct k_work_q
 *  3. Call k_work_q_start() in main()
 * =========================================================================
 */
#define CUSTOM_WQ_STACK_SIZE 1024
#define CUSTOM_WQ_PRIORITY   5

K_THREAD_STACK_DEFINE(custom_wq_stack, CUSTOM_WQ_STACK_SIZE);
static struct k_work_q custom_work_q;

/* =========================================================================
 * Shared processing counter (incremented by result_work_fn)
 * =========================================================================
 */
static atomic_t process_count;

/* =========================================================================
 * WORK ITEM 1 — sensor_work (k_work, system work queue)
 *
 * k_work is the simplest work item: fire-and-forget, submitted to a
 * work queue, runs once, then is "idle" again.
 *
 * This work item simulates processing a sensor reading.  It is submitted
 * from the button GPIO ISR (see button_isr below) — the classic pattern
 * for deferring ISR work to a thread context where sleeping and logging
 * are allowed.
 * =========================================================================
 */
static void sensor_work_fn(struct k_work *work)
{
	/*
	 * We are now in THREAD context (the system work queue thread).
	 * It is safe to call LOG_INF, k_msleep, etc.
	 */
	LOG_INF("sensor_work: processing button-triggered sensor read "
		"(count=%d)", (int)atomic_get(&process_count));

	/* Simulate some processing time */
	k_msleep(10);

	/* Toggle LED to give visual feedback */
	gpio_pin_toggle_dt(&led);

	atomic_inc(&process_count);
}

/* Declare the work item — K_WORK_DEFINE or runtime k_work_init both work */
static K_WORK_DEFINE(sensor_work, sensor_work_fn);

/* =========================================================================
 * WORK ITEM 2 — result_work (k_work, system work queue)
 *
 * Submitted by periodic_dwork_fn after it finishes processing.
 * Demonstrates work chaining: one work item submitting another.
 * =========================================================================
 */
static void result_work_fn(struct k_work *work)
{
	LOG_INF("result_work: posting result to system wq "
		"(total processed=%d)", (int)atomic_get(&process_count));
}

static K_WORK_DEFINE(result_work, result_work_fn);

/* =========================================================================
 * WORK ITEM 3 — periodic_dwork (k_work_delayable, custom work queue)
 *
 * k_work_delayable wraps a k_work and adds a delay before it runs.
 * To create a periodic effect, reschedule it from within its own handler
 * using k_work_reschedule_for_queue().
 *
 * Key API:
 *   k_work_reschedule_for_queue(wq, dwork, delay) — submit with delay
 *   k_work_reschedule(dwork, delay)               — submit to its last queue
 *   k_work_cancel_delayable(dwork)                — cancel a pending delay
 *   k_work_delayable_is_pending(dwork)            — check if scheduled
 * =========================================================================
 */
#define PERIODIC_INTERVAL_MS 2000

static void periodic_dwork_fn(struct k_work *work)
{
	/*
	 * Recover the containing k_work_delayable from the embedded k_work.
	 * This is the standard pattern when you need the delayable handle
	 * inside the handler.
	 */
	struct k_work_delayable *dwork =
		k_work_delayable_from_work(work);

	static uint32_t fire_count;

	fire_count++;
	LOG_INF("periodic_dwork: fired #%u (custom wq, every %d ms)",
		fire_count, PERIODIC_INTERVAL_MS);

	/*
	 * Chain: submit result_work to the SYSTEM work queue.
	 *
	 * k_work_is_pending() returns true if the work item is already
	 * queued or running.  Guard against submitting while still running.
	 */
	if (!k_work_is_pending(&result_work)) {
		k_work_submit(&result_work); /* → system work queue */
	} else {
		LOG_WRN("periodic_dwork: result_work still pending, skipping");
	}

	/*
	 * Reschedule ourselves for the next period.
	 * k_work_reschedule_for_queue(queue, dwork, delay)
	 *   — submits dwork to 'queue' after 'delay'
	 *   — if already scheduled, resets the countdown
	 */
	k_work_reschedule_for_queue(&custom_work_q, dwork,
				    K_MSEC(PERIODIC_INTERVAL_MS));
}

/* K_WORK_DELAYABLE_DEFINE initialises both the k_work and delay bookkeeping */
static K_WORK_DELAYABLE_DEFINE(periodic_dwork, periodic_dwork_fn);

/* =========================================================================
 * BUTTON GPIO ISR — ISR → work queue pattern
 *
 * GPIO interrupts run in interrupt context.  You CANNOT:
 *   - Call LOG_INF (may take a mutex internally)
 *   - Call k_msleep / k_sleep
 *   - Access most kernel objects that can block
 *
 * What you CAN do: call k_work_submit() — it is ISR-safe.
 * This is the canonical way to "escape" interrupt context.
 * =========================================================================
 */
static void button_isr(const struct device *dev,
		       struct gpio_callback *cb,
		       uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	/*
	 * ISR context — printk is ISR-safe, LOG_INF is NOT reliable here.
	 * Submit work to the system work queue; the actual processing
	 * happens in sensor_work_fn() in thread context.
	 *
	 * k_work_submit(work) — submit to the SYSTEM work queue.
	 * Returns: 0 if submitted, 1 if already queued (idempotent).
	 */
	printk("[ISR] button pressed — submitting sensor_work\n");
	k_work_submit(&sensor_work);
}

/* =========================================================================
 * main() — initialise hardware, start custom work queue, kick off periodic
 * =========================================================================
 */
int main(void)
{
	int ret;

	printk("\n========================================\n");
	printk("  Conexio Stratus Pro — Work Queues\n");
	printk("  Board: %s\n", CONFIG_BOARD);
	printk("========================================\n\n");

	/* --- LED setup --- */
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED: %d", ret);
		return ret;
	}

	/* --- Button setup --- */
	if (!gpio_is_ready_dt(&button)) {
		LOG_ERR("Button GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure button: %d", ret);
		return ret;
	}

	/*
	 * Configure GPIO interrupt: fire on the active edge (falling edge
	 * for active-low button with pull-up).
	 * GPIO_INT_EDGE_TO_ACTIVE = trigger when pin reaches its active state.
	 */
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure button interrupt: %d", ret);
		return ret;
	}

	/* Register the ISR callback */
	gpio_init_callback(&button_cb_data, button_isr,
			   BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	LOG_INF("Button on pin %d configured with edge interrupt", button.pin);

	/* -----------------------------------------------------------------------
	 * Start the CUSTOM work queue
	 *
	 * k_work_q_start(work_q, stack, stack_size, priority, cfg)
	 *   cfg = NULL for defaults (no name, no no_yield flag)
	 *
	 * After this call the custom work queue thread is running and ready
	 * to accept work items submitted with k_work_submit_to_queue().
	 * -----------------------------------------------------------------------
	 */
	k_work_queue_start(&custom_work_q,
			   custom_wq_stack,
			   K_THREAD_STACK_SIZEOF(custom_wq_stack),
			   CUSTOM_WQ_PRIORITY,
			   NULL); /* default config */

	k_thread_name_set(&custom_work_q.thread, "custom_wq");
	LOG_INF("Custom work queue started at priority %d", CUSTOM_WQ_PRIORITY);

	/* -----------------------------------------------------------------------
	 * Kick off the periodic delayable work item.
	 *
	 * k_work_reschedule_for_queue(queue, dwork, delay)
	 * First call: schedule the first fire after PERIODIC_INTERVAL_MS.
	 * Subsequent fires are self-rescheduled inside periodic_dwork_fn().
	 * -----------------------------------------------------------------------
	 */
	k_work_reschedule_for_queue(&custom_work_q,
				    &periodic_dwork,
				    K_MSEC(PERIODIC_INTERVAL_MS));

	LOG_INF("Periodic work scheduled every %d ms on custom wq",
		PERIODIC_INTERVAL_MS);

	/* -----------------------------------------------------------------------
	 * Main loop — demonstrates k_work_is_pending polling
	 *
	 * Every 3 seconds check whether periodic_dwork is still pending
	 * (scheduled but not yet run) or has already fired.
	 * -----------------------------------------------------------------------
	 */
	while (1) {
		k_msleep(3000);

		bool dwork_pending = k_work_delayable_is_pending(&periodic_dwork);

		LOG_INF("main loop: periodic_dwork pending=%s, "
			"total_processed=%d",
			dwork_pending ? "yes" : "no",
			(int)atomic_get(&process_count));
	}

	return 0;
}
