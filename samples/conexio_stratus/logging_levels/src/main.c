/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Logging Levels Sample for Conexio Stratus Pro (nRF9151)
 *
 * Demonstrates:
 *  - LOG_MODULE_REGISTER with LOG_LEVEL_DBG
 *  - All four log macros: LOG_ERR, LOG_WRN, LOG_INF, LOG_DBG
 *  - LOG_HEXDUMP_INF / LOG_HEXDUMP_DBG for binary data inspection
 *  - Runtime log level filtering with log_filter_set()
 *  - Deferred vs immediate logging (config comment)
 *  - LOG_MODULE_DECLARE for a "subsystem" within the same file
 *  - Structured demo: 5 iterations at full verbosity, then 5 with WRN filter
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

/* ── Primary module registration ────────────────────────────────────────
 *
 * LOG_MODULE_REGISTER(name, level) creates the log source for this
 * compilation unit.  Every .c file that wants to emit log messages must
 * call this macro exactly once at file scope.
 *
 * LOG_LEVEL_DBG enables all four severity levels (ERR/WRN/INF/DBG).
 * The runtime default is further constrained by CONFIG_LOG_DEFAULT_LEVEL.
 */
LOG_MODULE_REGISTER(app_main, LOG_LEVEL_DBG);

/* ── Simulated "sensor" subsystem ───────────────────────────────────────
 *
 * In a real project each subsystem lives in its own .c file and calls
 * LOG_MODULE_REGISTER.  Within a single file we can use LOG_MODULE_DECLARE
 * to switch the active module for a group of functions.  The macro must
 * appear at file scope (not inside a function).
 *
 */

/* ── Sample binary data for hexdump demonstrations ──────────────────── */
static const uint8_t sensor_frame[] = {
	0xAA, 0xBB, 0x01, 0x02, 0x03, 0xFF, 0x7F, 0x00,
	0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE
};

/* ════════════════════════════════════════════════════════════════════════
 * Subsystem helper — simulates a sensor read with realistic log messages
 * ════════════════════════════════════════════════════════════════════════ */

/**
 * @brief Simulate a sensor read cycle, emitting logs at all four levels.
 *
 * @param iteration  Current loop iteration (0-based).
 */
static void sensor_read_cycle(int iteration)
{
	/* Simulated ADC raw value and converted millivolt reading */
	uint16_t raw_adc = (uint16_t)(0x0800 + iteration * 0x10);
	int mv        = (raw_adc * 3300) / 4096;

	/* Simulated sensor value — goes out of range on iteration 3 */
	int sensor_val = 50 + iteration * 15;

	/* LOG_DBG — lowest priority; fine-grained developer detail.
	 * Only visible when the runtime level is 4 (DBG). */
	LOG_DBG("Raw ADC value: 0x%04X, converted: %d mV", raw_adc, mv);

	/* LOG_INF — normal operational status. */
	LOG_INF("Iteration %d: sensor value = %d, uptime = %lld ms",
		iteration, sensor_val, k_uptime_get());

	/* LOG_WRN — unexpected but non-fatal condition. */
	if (sensor_val > 90) {
		LOG_WRN("Sensor value %d out of expected range [%d-%d]",
			sensor_val, 0, 90);
	}

	/* LOG_ERR — serious problem that requires attention.
	 * Triggered here on iteration 4 to demonstrate error output. */
	if (iteration == 4) {
		LOG_ERR("Failed to initialise sensor on iteration %d, err %d",
			iteration, -EIO);
	}
}

/* ════════════════════════════════════════════════════════════════════════
 * Runtime level change helper
 *
 * log_filter_set() changes the minimum severity level that the logging
 * subsystem forwards to backends at runtime without recompiling.
 *
 * Parameters:
 *   backend  – NULL targets all backends simultaneously
 *   domain   – Z_LOG_LOCAL_DOMAIN_ID for the local (single) domain
 *   source   – numeric log source ID obtained from log_source_id_get()
 *   level    – new minimum level (LOG_LEVEL_ERR … LOG_LEVEL_DBG)
 * ════════════════════════════════════════════════════════════════════════ */

static void set_runtime_level(uint8_t level)
{
	uint32_t source_id = log_source_id_get("app_main");

	log_filter_set(NULL,               /* all backends */
		       Z_LOG_LOCAL_DOMAIN_ID,
		       source_id,
		       level);
}

/* ════════════════════════════════════════════════════════════════════════
 * main
 * ════════════════════════════════════════════════════════════════════════ */

int main(void)
{
	printk("\n");
	printk("=================================================\n");
	printk("  Conexio Stratus Pro — Logging Levels Demo\n");
	printk("  Board: %s\n", CONFIG_BOARD);
	printk("=================================================\n");
	printk("\n");
	printk("Log level guide:\n");
	printk("  LOG_LEVEL_ERR (1) — critical failures only\n");
	printk("  LOG_LEVEL_WRN (2) — warnings + errors\n");
	printk("  LOG_LEVEL_INF (3) — normal status + above\n");
	printk("  LOG_LEVEL_DBG (4) — all messages (verbose)\n");
	printk("\n");
	printk("CONFIG_LOG_DEFAULT_LEVEL=%d (set in prj.conf)\n",
	       CONFIG_LOG_DEFAULT_LEVEL);
	printk("\n");

	/*
	 * Deferred vs Immediate logging
	 * ──────────────────────────────
	 * CONFIG_LOG_MODE_DEFERRED (default):
	 *   Log calls return immediately.  A background thread collects
	 *   buffered messages and writes them to backends.  This minimises
	 *   the latency impact on the calling thread at the cost of a small
	 *   delay before messages appear.
	 *
	 * CONFIG_LOG_MODE_IMMEDIATE:
	 *   Log calls block until the message has been fully emitted to all
	 *   active backends.  Useful for debugging hard faults or boot
	 *   failures where deferred output might be lost.  Has higher
	 *   per-call overhead.
	 *
	 * This sample uses deferred mode (the default).
	 */

	/* ── Phase 1: Full verbosity (DBG) — 5 iterations ──────────────── */

	LOG_INF("=== Phase 1: Running at LOG_LEVEL_DBG ===");
	LOG_INF("System started, uptime: %lld ms", k_uptime_get());

	for (int i = 0; i < 5; i++) {
		sensor_read_cycle(i);
		k_msleep(200);
	}

	/* ── Hexdump demonstration ──────────────────────────────────────── */

	LOG_INF("--- Hexdump demo (INF level) ---");
	LOG_HEXDUMP_INF(sensor_frame, sizeof(sensor_frame),
			"Sensor frame bytes:");

	LOG_DBG("--- Hexdump demo (DBG level) ---");
	LOG_HEXDUMP_DBG(sensor_frame, sizeof(sensor_frame),
			"Same frame at DBG:");

	/* ── Phase 2: Change runtime level to WRN, run 5 more iterations ─ */

	LOG_WRN("=== Phase 2: Switching runtime level to LOG_LEVEL_WRN ===");
	LOG_INF("(This INF message is the last you will see until level is restored)");

	/* Give deferred logger time to flush Phase 1 messages */
	k_msleep(100);

	set_runtime_level(LOG_LEVEL_WRN);

	LOG_DBG("This DBG message is now suppressed at runtime");
	LOG_INF("This INF message is now suppressed at runtime");

	for (int i = 0; i < 5; i++) {
		sensor_read_cycle(i);
		k_msleep(200);
	}

	/* ── Restore to DBG and confirm ─────────────────────────────────── */

	set_runtime_level(LOG_LEVEL_DBG);

	LOG_INF("=== Phase 3: Level restored to LOG_LEVEL_DBG ===");
	LOG_DBG("DBG messages are visible again");
	LOG_INF("Total uptime: %lld ms", k_uptime_get());

	LOG_INF("Logging demo complete. Halting.");

	return 0;
}
