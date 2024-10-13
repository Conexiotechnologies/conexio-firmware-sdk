/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include "fuel_gauge.h"

#define SLEEP_TIME_MS 2000

#define NPM1300_CHGR_BASE 0x3
#define NPM1300_CHGR_OFFSET_DIS_SET 0x06

static const struct device *npm1300_mfd_dev = DEVICE_DT_GET(DT_INST(0, nordic_npm1300));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(pmic_charger));

static int npm1300_mfd_init(void)
{
   int ret = 0;

   if (!device_is_ready(npm1300_mfd_dev)) {
		printk("NPM1300 mfd not ready!");
		return -ENOTSUP;
	}

#ifdef CONFIG_MFD_NPM1300_DISABLE_NTC
   ret = mfd_npm1300_reg_write(npm1300_mfd_dev, NPM1300_CHGR_BASE, NPM1300_CHGR_OFFSET_DIS_SET, 2);
	if (ret < 0) {
		return ret;
	}
#endif /* CONFIG_MFD_NPM1300_DISABLE_NTC */

   return ret;
}

int main(void)
{
	printk("nPM1300 PMIC sample running on %s\n", CONFIG_BOARD);

	npm1300_mfd_init();

	if (!device_is_ready(charger)) {
		printk("Charger device not ready.\n");
		return 0;
	}

	if (fuel_gauge_init(charger) < 0) {
		printk("Could not initialise fuel gauge.\n");
		return 0;
	}

	printk("PMIC device init successful\n");

	while (1) {
		fuel_gauge_update(charger);
		k_msleep(SLEEP_TIME_MS);
	}
}
