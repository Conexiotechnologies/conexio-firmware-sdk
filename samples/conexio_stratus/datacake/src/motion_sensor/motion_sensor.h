/*
 * Copyright (c) 2024 Conexio Technologies, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOTION_SENSOR_H_
#define MOTION_SENSOR_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Data obtained from the device when an event occurs.
 *
 */
struct motion_data
{
    /* Value of XYZ when sample was taken */
    struct sensor_value x, y, z;
};

/**
 * @brief Gets a sample from the LIS2DH sensor
 *
 * @param p_data pointer to the data structure being filled
 * @return int 0 on success
 */
int motion_sensor_sample_fetch(struct motion_data *p_data);

/**
 * @brief Initialize the motion sensors.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */

int motion_sensor_init(void);

#endif /* MOTION_SENSOR_H_ */
