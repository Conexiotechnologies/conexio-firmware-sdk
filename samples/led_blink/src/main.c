/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2021 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN		DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN		0
#define FLAGS	0
#endif

#if defined(CONFIG_STRATUS_SHIELD)
/* The devicetree node identifier for the "led1" alias. */
#define LED1_NODE DT_ALIAS(led1)
#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
#define LED1	DT_GPIO_LABEL(LED1_NODE, gpios)
#define PIN1	DT_GPIO_PIN(LED1_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an Orange LED 1. */
#error "Unsupported board: led1 devicetree alias is not defined"
#define LED1	""
#define PIN1	0
#endif
#endif

#ifndef FLAGS
#define FLAGS	0
#endif

void main(void)
{
	printk("Hello from %s\n", CONFIG_BOARD);

	const struct device *led_0;
	bool led_is_on = true;
	int ret_0;

#if defined(CONFIG_STRATUS_SHIELD)
	const struct device *led_1;
	int ret_1;
#endif

	led_0 = device_get_binding(LED0);
	if (led_0 == NULL) {
		return;
	}

#if defined(CONFIG_STRATUS_SHIELD)
	led_1 = device_get_binding(LED1);
	if (led_1 == NULL) {
		return;
	}
#endif

	ret_0 = gpio_pin_configure(led_0, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret_0 < 0) {
		return;
	}

#if defined(CONFIG_STRATUS_SHIELD)
	ret_1 = gpio_pin_configure(led_1, PIN1, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret_1 < 0) {
		return;
	}
#endif

    /* main loop */
	while (1) {
		/* LED 0 on main board */
		gpio_pin_set(led_0, PIN, (int)led_is_on);

#if defined(CONFIG_STRATUS_SHIELD)
		/* LED 1 on Stratus shield */
		gpio_pin_set(led_1, PIN1, (int)led_is_on);
#endif

		led_is_on = !led_is_on;
		k_msleep(SLEEP_TIME_MS);
	}
}
