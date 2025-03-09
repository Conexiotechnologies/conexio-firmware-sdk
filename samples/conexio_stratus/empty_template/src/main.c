/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
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
