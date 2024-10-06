/*
 * Copyright (c) 2021 Leonard Pollak
 * Copyright (c) 2024 Conexio Technologies, Inc
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include <zephyr/drivers/sensor/sht4x.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

int main(void)
{
	printf("SHT4X sensor sample running on %s\n", CONFIG_BOARD);

	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	struct sensor_value temp, hum;

	if (!device_is_ready(sht)) {
		printf("Device %s is not ready.\n", sht->name);
		return 0;
	}

	while (true) {

		if (sensor_sample_fetch(sht)) {
			printf("Failed to fetch sample from SHT4X device\n");
			return 0;
		}

		sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);

		printf("SHT4X: Temp: %d.%06d degC; RH: %d.%06d %%\n", temp.val1, temp.val2, hum.val1, hum.val2);

		k_sleep(K_MSEC(2000));
	}
}
