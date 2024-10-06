/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

int main(void)
{
	printk("Empty template sample\r\n");

	while (1) {
		printk("Hello from %s\n", CONFIG_BOARD);
		k_msleep(5000);
	}

	return 0;
}
