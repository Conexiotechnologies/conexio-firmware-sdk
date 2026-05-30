/*
 * Copyright (c) 2025 Conexio Technologies, Inc
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * nPM1300 LED sample for Conexio Stratus Pro (nRF9151)
 *
 * The nPM1300 exposes three LED outputs:
 *   LED0 — "charging" mode: driven automatically by the charger state machine
 *   LED1 — "error"    mode: driven automatically by PMIC error conditions
 *   LED2 — "host"     mode: fully software-controlled via the Zephyr LED API
 *
 * This sample demonstrates:
 *   1. How to obtain the nPM1300 LED device from the devicetree.
 *   2. How to turn individual LEDs on/off using led_on() / led_off().
 *   3. A simple blink pattern that sequences through all three LEDs and
 *      then blinks LED2 (host-mode) continuously.
 *
 * Note: led_on() / led_off() only have an observable effect on LEDs
 * configured in "host" mode. Calls on LEDs in "charging" or "error"
 * mode return -ENOTSUP and are safely ignored here.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(npm_leds, LOG_LEVEL_INF);

/* Devicetree node for the nPM1300 LED controller */
#define NPM1300_LEDS_NODE DT_NODELABEL(npm1300_leds)

/* LED indices matching the DT definition order */
#define LED0_CHARGING 0U   /* "charging" mode — auto-controlled */
#define LED1_ERROR    1U   /* "error"    mode — auto-controlled */
#define LED2_HOST     2U   /* "host"     mode — software-controlled */

/* Blink interval from Kconfig (default 500 ms) */
#define BLINK_MS CONFIG_NPM_LEDS_BLINK_INTERVAL_MS

static const struct device *led_dev = DEVICE_DT_GET(NPM1300_LEDS_NODE);

/**
 * @brief Try to turn a LED on, log result.
 *
 * LEDs in "charging" or "error" mode will return -ENOTSUP — this is
 * expected and logged as informational rather than an error.
 */
static void led_set(uint32_t idx, bool on)
{
	int ret = on ? led_on(led_dev, idx) : led_off(led_dev, idx);

	if (ret == 0) {
		LOG_INF("LED%u %s", idx, on ? "ON" : "OFF");
	} else if (ret == -ENOTSUP) {
		LOG_INF("LED%u is in auto mode (charging/error), skipping software control", idx);
	} else {
		LOG_ERR("LED%u %s failed: %d", idx, on ? "on" : "off", ret);
	}
}

/**
 * @brief Turn the user LED (LED2, host mode) on or off.
 *
 * This is the intended entry point for application code that wants to
 * use the nPM1300 LED2 output as a general-purpose status indicator.
 *
 * @param on  true to turn LED2 on, false to turn it off.
 */
static void user_led_set(bool on)
{
	led_set(LED2_HOST, on);
}

/**
 * @brief Sequence all three LEDs on then off, one at a time.
 */
static void led_chase_sequence(void)
{
	LOG_INF("--- Chase sequence: LED0 -> LED1 -> LED2 ---");

	for (uint32_t i = 0; i < 3; i++) {
		led_set(i, true);
		k_msleep(BLINK_MS);
		led_set(i, false);
		k_msleep(BLINK_MS / 2);
	}
}

int main(void)
{
	printk("nPM1300 LED sample running on %s\n", CONFIG_BOARD);

	if (!device_is_ready(led_dev)) {
		LOG_ERR("nPM1300 LED device not ready");
		return -ENODEV;
	}

	LOG_INF("nPM1300 LED device ready");
	LOG_INF("LED0 = charging (auto), LED1 = error (auto), LED2 = host (software)");

	/*
	 * Run one pass of the chase sequence so all three outputs are
	 * exercised — useful for board bring-up / visual confirmation.
	 */
	led_chase_sequence();

	k_msleep(1000);

	/*
	 * Main loop: blink LED2 (host mode) continuously.
	 * LED0 and LED1 are managed automatically by the PMIC hardware.
	 */
	LOG_INF("Starting continuous blink on LED2 (host mode)");

	bool state = false;

	while (1) {
		state = !state;
		user_led_set(state);
		k_msleep(BLINK_MS);
	}

	return 0;
}
