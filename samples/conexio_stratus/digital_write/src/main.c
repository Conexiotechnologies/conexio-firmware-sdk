/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>

#define PINOUT DT_ALIAS(gpiopin)
static struct gpio_dt_spec output_pin = GPIO_DT_SPEC_GET(PINOUT, gpios);

int main(void)
{
	printf("Gpio output sample running on %s\n", CONFIG_BOARD);

	// Configure gpio pin as output and low
	gpio_pin_configure_dt(&output_pin, GPIO_OUTPUT_INACTIVE);

	while (true) {
        gpio_pin_set_dt(&output_pin, 1);
		k_sleep(K_MSEC(2000));
        gpio_pin_set_dt(&output_pin, 0);
	}
}