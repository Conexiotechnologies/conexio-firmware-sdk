/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/sys/poweroff.h>

int main(void)
{
	int err;

	printk("Low Power Mode sample has started\n");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Failed to initialize modem library, error: %d\n", err);
		return -1;
	}

	/* Turn LTE power off */
	lte_lc_power_off();

	k_sleep(K_MSEC(1000));

	/* Shut down the nRF91 regulator */
	NRF_REGULATORS->SYSTEMOFF = 1;
	
	/* Shut down modem library */
	err = nrf_modem_lib_shutdown();
	if (err) {
		return -1;
	}

	/* Finally poweroff the system safely */
	sys_poweroff();

	return 0;
}
