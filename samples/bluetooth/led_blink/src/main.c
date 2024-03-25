/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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
#define RED_LED DT_ALIAS(led0)

#define LED_OFF 0
#define LED_ON !LED_OFF

static struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(RED_LED, gpios);
static const struct device *gpio_dev;

LOG_MODULE_REGISTER(app);

void init_led(void)
{
	gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
}

int main(void)
{
	gpio_dev = DEVICE_DT_GET(GPIO_NODE);

	if (!gpio_dev) {

		printk("Error getting GPIO device binding\r\n");

		return false;
	}

	init_led();

	while (1) {
		printk("Hello from %s\n", CONFIG_BOARD);
		printk("Let's blink the LED\n");
		// Blink the red LED on board
		gpio_pin_set_dt(&red_led, LED_ON);
		k_msleep(1000);
		gpio_pin_set_dt(&red_led, LED_OFF);
		k_msleep(1000);
	}

	return 0;
}