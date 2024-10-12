/*
 * Copyright (c) 2024 Conexio Technologies, Inc.
 * 
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <motion_sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(motion_sensor, CONFIG_MQTT_DATACAKE_LOG_LEVEL);

static const struct device *sensor = DEVICE_DT_GET_ANY(st_lis2dh);

int motion_sensor_sample_fetch(struct motion_data *p_data)
{
    int err;
    struct sensor_value accel[3];

    err = sensor_sample_fetch_chan(sensor, SENSOR_CHAN_ACCEL_XYZ);
    if (err)
    {
        LOG_ERR("Unable to fetch motion sensor data. error: %d", err);
        return err;
    }

    err = sensor_channel_get(sensor, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (err)
    {
        LOG_ERR("Unable to get motion sensor data. error: %d", err);
        return err;
    }

    p_data->x = accel[0];
    p_data->y = accel[1];
    p_data->z = accel[2];

    return 0;
}

int motion_sensor_init(void)
{
    if (sensor == NULL) {
        LOG_ERR("No motion sensor found\n");
        return -ENODEV;
	}

    if (!device_is_ready(sensor))
    {
        LOG_ERR("Motion sensor %s is not ready\n", sensor->name);
        return -ENODEV;
    }

    LOG_INF("Motion sensor initialized");

    return 0;
}
