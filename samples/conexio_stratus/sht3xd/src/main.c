/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

int main(void)
{
	printf("SHT3XD sensor sample running on %s\n", CONFIG_BOARD);

	const struct device *const dev = DEVICE_DT_GET_ONE(sensirion_sht3xd);
	int rc;

	if (!device_is_ready(dev)) {
		printf("Device %s is not ready\n", dev->name);
		return 0;
	}

	while (true) {
		struct sensor_value temp, hum;

		rc = sensor_sample_fetch(dev);
		if (rc == 0) {
			rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP,
						&temp);
		}
		if (rc == 0) {
			rc = sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY,
						&hum);
		}
		if (rc != 0) {
			printf("SHT3XD: failed: %d\n", rc);
			break;
		}

		float temp_celsius = sensor_value_to_double(&temp);
		float temp_fahrenheit = (temp_celsius * 9.0 / 5.0) + 32.0;

		printf("SHT3XD: %.2f °C ; %.2f °F ; %.2f %%RH\n",
				temp_celsius,
				temp_fahrenheit,
				sensor_value_to_double(&hum));

		k_sleep(K_MSEC(2000));
	}
	return 0;
}
