/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#define GPIO_NODE	DT_NODELABEL(gpio0)

/* The devicetree node identifier for the "led0" alias. */
#define LED DT_ALIAS(led0)

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED, gpios);
static const struct device *gpio_dev;

LOG_MODULE_REGISTER(app);

/* Set the sleep time */
#define SLEEP_TIME_MS 1000

void init_led(void)
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}

int main(void)
{
	printk("Hello from %s\n", CONFIG_BOARD);

	gpio_dev = DEVICE_DT_GET(GPIO_NODE);
	if (!gpio_dev) {
		printk("Error getting GPIO device binding\r\n");
		return false;
	}

	init_led();

	while (1) {
		// Blink the LED on Stratus board
		gpio_pin_toggle_dt(&led);
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
