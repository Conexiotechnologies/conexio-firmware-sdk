/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2025 Conexio Technologies, Inc
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include "fuel_gauge.h"

#define SLEEP_TIME_MS 1000
#define NPM1300_CHGR_BASE 0x3
#define NPM1300_CHGR_OFFSET_DIS_SET 0x06

static const struct device *pmic = DEVICE_DT_GET(DT_INST(0, nordic_npm1300));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(pmic_charger));
static volatile bool vbus_connected;

static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED)) {
		printk("Vbus connected\n");
		vbus_connected = true;
	}

	if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED)) {
		printk("Vbus removed\n");
		vbus_connected = false;
	}
}

int main(void)
{
	printk("nPM1300 PMIC sample running on %s\n", CONFIG_BOARD);

	int err;

	if (!device_is_ready(pmic)) {
		printk("Pmic device not ready.\n");
		return 0;
	}

#ifdef CONFIG_MFD_NPM1300_DISABLE_NTC
	err = mfd_npm1300_reg_write(pmic, NPM1300_CHGR_BASE, NPM1300_CHGR_OFFSET_DIS_SET, 2);
	if (err) {
		printk("Failed to disable NTC.\n");
		return err;
	}
#endif /* CONFIG_MFD_NPM1300_DISABLE_NTC */

	if (!device_is_ready(charger)) {
		printk("Charger device not ready.\n");
		return 0;
	}

	if (fuel_gauge_init(charger) < 0) {
		printk("Could not initialise fuel gauge.\n");
		return 0;
	}

	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback,
				   BIT(NPM1300_EVENT_VBUS_DETECTED) |
				   BIT(NPM1300_EVENT_VBUS_REMOVED));

	err = mfd_npm1300_add_callback(pmic, &event_cb);
	if (err) {
		printk("Failed to add pmic callback.\n");
		return 0;
	}

	/* Initialise vbus detection status. */
	struct sensor_value val;
	int ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);

	if (ret < 0) {
		return false;
	}

	vbus_connected = (val.val1 != 0) || (val.val2 != 0);

	printk("PMIC device initialized successful\n");

	while (1) {
		fuel_gauge_update(charger, vbus_connected);
		k_msleep(SLEEP_TIME_MS);
	}
}
