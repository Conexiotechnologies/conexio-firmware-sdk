/*---------------------------------------------------------------------------*/
/*
 * Copyright (c) 2022 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */   
/*---------------------------------------------------------------------------*/
#include <zephyr.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include "person_sensor.h"
#include <logging/log.h>
/*---------------------------------------------------------------------------*/
/* Register the logging module */
LOG_MODULE_REGISTER(app, CONFIG_USEFULSENSORS_LOG_LEVEL);
/*---------------------------------------------------------------------------*/
const struct device *i2c_device;
/*---------------------------------------------------------------------------*/
static int rc;
static int i;
/*---------------------------------------------------------------------------*/
// How long to wait between sensor sampling. The sensor can be read as
// frequently as you like, but the results only change at about 5FPS, so
// waiting for >200ms is reasonable. Here, we are waiting for 1 sec. 
#define SAMPLING_INTERVAL_MS   (K_MSEC(1000))
/*---------------------------------------------------------------------------*/
/*
 * Brief : Fetch the latest results from the sensor
 * Param ; Pointer to the results
 * Return : false if the read didn't succeed.
 */
bool person_sensor_read(person_sensor_results_t* results) {
    rc = i2c_read(i2c_device, (uint8_t*)(results), sizeof(person_sensor_results_t), PERSON_SENSOR_I2C_ADDRESS);
    if (rc < 0) {
        LOG_ERR("i2c_read error: %d", rc);
        return false;
	} else {
        return true;
    }
}
/*---------------------------------------------------------------------------*/
/*
 * Brief : Writes the value to the sensor register
 * Param1 : Register address
 * Param2 : values to be written
 * Return : void
 */
void person_sensor_write_reg(uint8_t reg, uint8_t value) {
    uint8_t write_bytes[2] = {reg, value};

    rc = i2c_write(i2c_device, write_bytes, sizeof(write_bytes), PERSON_SENSOR_I2C_ADDRESS);
    if (rc < 0) {
        LOG_ERR("i2c_write error: %d", rc);
	}
}
/*---------------------------------------------------------------------------*/
/*
 * Brief : Initialize the I2C bus
 * Param : I2C device 
 * Return : -EINVAL if failed, else 0 if Successful
 */
static int i2c_init(void) {
    LOG_INF("Setting up i2c");

    i2c_device = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    
    if (!device_is_ready(i2c_device)) {
		LOG_ERR("I2C: Device is not ready");
		return -EINVAL;
	}

    int ret = i2c_configure(i2c_device, I2C_SPEED_SET(I2C_SPEED_FAST));
    if (ret < 0 )
    {
        LOG_ERR("I2C: Configuration failed");
        return ret;
    }

    return 0;
}
/*---------------------------------------------------------------------------*/
void main(void) 
{
	LOG_INF("Usefulsensors person sensor sample started"); 
    person_sensor_results_t results = {};

    i2c_init();
    
    while (1)
    {
        // Perform a read action on the I2C address of the sensor to get the
        // current face information detected.
        if (!person_sensor_read(&results)) 
        {
            LOG_ERR("No person sensor results found on the i2c bus");
            k_sleep(SAMPLING_INTERVAL_MS);
            return;
        }

        LOG_INF("****************");
        LOG_INF("%d face(s) found", results.num_faces);

        for (i = 0; i < results.num_faces; ++i) 
        {
            const person_sensor_face_t* face = &results.faces[i];

            LOG_INF("Face #%d: %d confidence, (%d, %d), (%d, %d), %s",
            i, face->box_confidence, face->box_left, face->box_top,
            face->box_right, face->box_bottom, 
            face->is_facing ? "facing" : "not facing");
        }
        k_sleep(SAMPLING_INTERVAL_MS);
    }
}
/*---------------------------------------------------------------------------*/