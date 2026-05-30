/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Digital Read/Write GPIO sample for Conexio Stratus Pro (nRF9151)
 *
 * Demonstrates simultaneous use of a digital input and a digital output
 * on the Stratus header connector using the Zephyr GPIO API.
 *
 * Pin assignment (configured via DT overlay):
 *   Input  — P0.18 (alias: pininput)  — pulled up, active-low
 *   Output — P0.24 (alias: pinoutput) — active-high, starts low
 *
 * Behaviour:
 *   Every POLL_INTERVAL_MS the input pin is sampled. The output pin mirrors
 *   the input state — high when input is asserted, low otherwise. Both states
 *   are logged to the serial console.
 *
 * To test without external hardware:
 *   Connect P0.18 to GND to assert the input and observe the output on P0.24
 *   with a multimeter or logic analyser.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(digital_rw, LOG_LEVEL_INF);

/* How often to poll the input pin */
#define POLL_INTERVAL_MS 1000

/* Devicetree aliases defined in the overlay */
#define INPUT_NODE  DT_ALIAS(pininput)
#define OUTPUT_NODE DT_ALIAS(pinoutput)

static const struct gpio_dt_spec input_pin  = GPIO_DT_SPEC_GET(INPUT_NODE,  gpios);
static const struct gpio_dt_spec output_pin = GPIO_DT_SPEC_GET(OUTPUT_NODE, gpios);

int main(void)
{
	int ret;

	printk("Digital read/write sample running on %s\n", CONFIG_BOARD);

	/* --- Validate devices --- */
	if (!device_is_ready(input_pin.port)) {
		LOG_ERR("Input GPIO port not ready");
		return -ENODEV;
	}

	if (!device_is_ready(output_pin.port)) {
		LOG_ERR("Output GPIO port not ready");
		return -ENODEV;
	}

	/* --- Configure input pin --- */
	ret = gpio_pin_configure_dt(&input_pin, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure input pin P0.%d (err %d)",
			input_pin.pin, ret);
		return ret;
	}
	LOG_INF("Input pin P0.%d configured (pull-up, active-low)", input_pin.pin);

	/* --- Configure output pin, start inactive (low) --- */
	ret = gpio_pin_configure_dt(&output_pin, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure output pin P0.%d (err %d)",
			output_pin.pin, ret);
		return ret;
	}
	LOG_INF("Output pin P0.%d configured (active-high, starts low)", output_pin.pin);

	/* --- Main loop: read input, mirror to output --- */
	while (1) {
		int val = gpio_pin_get_dt(&input_pin);

		if (val < 0) {
			LOG_ERR("Error reading input pin P0.%d (err %d)",
				input_pin.pin, val);
		} else {
			LOG_INF("Input  P0.%d = %d (%s)",
				input_pin.pin, val,
				val ? "ASSERTED" : "idle");

			/* Mirror input state to output */
			ret = gpio_pin_set_dt(&output_pin, val);
			if (ret != 0) {
				LOG_ERR("Error setting output pin P0.%d (err %d)",
					output_pin.pin, ret);
			} else {
				LOG_INF("Output P0.%d = %d",
					output_pin.pin, val);
			}
		}

		k_msleep(POLL_INTERVAL_MS);
	}

	return 0;
}
