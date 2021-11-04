/*
 * Copyright (c) 2021 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

/* Grove buzzer connected to Stratus Shield (GPIO 27) */
#define GPIO0 DT_LABEL(DT_NODELABEL(gpio0))
#define GPIO_BUZZER 	27

void main(void)
{
	printk("Conexio Stratus Grove Buzzer \n");

	/* Get the device gpio binding */
	const struct device *gpio = device_get_binding(GPIO0);
	if (gpio == NULL) {
		return;
	}

	/* Configure buzzer signal pin as output */
	gpio_pin_configure(gpio, GPIO_BUZZER, GPIO_OUTPUT_LOW);

	while (1)
	{   
		/* Turn ON the buzzer */
		gpio_pin_set(gpio, GPIO_BUZZER, 1);
		k_sleep(K_MSEC(500));
		/* Turn OFF the buzzer */
		gpio_pin_set(gpio, GPIO_BUZZER, 0);
		/* Sleep for 5 seconds */
		k_sleep(K_MSEC(5000));
	}
}
