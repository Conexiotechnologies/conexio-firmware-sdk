/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

int main(void)
{
	printk("Conexio Stratus DPS310 sensor example\n");
	const struct device *const dev = DEVICE_DT_GET_ONE(infineon_dps310);

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return 0;
	}

	printk("dev %p name %s\n", dev, dev->name);

	while (1) {
		struct sensor_value temp, press;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
		
		printk("Temp: %d.%06d degC; Presssure: %d.%06d kPa\n",
		      temp.val1, abs(temp.val2), press.val1, press.val2);

		/* Fetch sensor samples every 3 seconds */
		k_sleep(K_MSEC(3000));
	}
	return 0;
}
