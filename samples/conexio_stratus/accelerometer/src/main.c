/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[3];
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor,
					SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}
	if (rc < 0) {
		printf("ERROR: Update failed: %d\n", rc);
	} else {
		printf("#%u, uptime: %u ms, x: %f, y: %f, z: %f\n",
		       count, k_uptime_get_32(),
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));
	}
}

int main(void)
{
	printf("Conexio Stratus accelerometer sensor example\n");
	const struct device *sensor = DEVICE_DT_GET_ANY(st_lis2dh);

	if (sensor == NULL) {
		printf("No device found\n");
		return 0;
	}
	if (!device_is_ready(sensor)) {
		printf("Device %s is not ready\n", sensor->name);
		return 0;
	}

	printf("Polling at 0.5 Hz\n");

	while (true) {
		fetch_and_display(sensor);
		k_sleep(K_MSEC(2000));
	}
}
