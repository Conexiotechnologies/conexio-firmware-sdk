/*
 * Copyright (c) 2019 Infineon Technologies AG
 * Copyright (c) 2021 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <devicetree.h>
#include <stdlib.h>

void main(void)
{
	printk("Conexio Stratus DPS310 sensor example");
	
    /* Get the device binding for the DPS310 */
	const struct device *dev = device_get_binding(DT_LABEL(DT_INST(0, infineon_dps310)));

	if (dev == NULL) {
		printk("Could not get DPS310 device\n");
		return;
	}

	printk("dev %p name %s\n", dev, dev->name);

	while (1) {
		/* Fetch sensor samples every 3 seconds */
		k_sleep(K_MSEC(3000));

		struct sensor_value temp, press;

		sensor_sample_fetch(dev);
		sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);

		printk("Temp: %d.%06d degC; Presssure: %d.%06d kPa\n",
		      temp.val1, abs(temp.val2), press.val1, press.val2);
	}
}
