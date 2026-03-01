/*
 * Copyright (c) 2026 Conexio Technologies, Inc
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <stdio.h>

/* Get the device structure from the device tree */
static const struct device *const scd41_dev = DEVICE_DT_GET_ANY(sensirion_scd41);

int main(void)
{
    printf("SCD4x sensor sample running on %s\n", CONFIG_BOARD);
	
    if (!device_is_ready(scd41_dev)) {
        printf("SCD41 device not ready\n");
        return 0;
    }

    printf("SCD41 device is ready!\n");

    while (1) {
        struct sensor_value co2_val, temp_val, hum_val;
        int rc = sensor_sample_fetch(scd41_dev);

        if (rc != 0) {
            printf("Sensor fetch failed: %d\n", rc);
        } else {
            /* Get sensor channels */
            sensor_channel_get(scd41_dev, SENSOR_CHAN_CO2, &co2_val);
            sensor_channel_get(scd41_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp_val);
            sensor_channel_get(scd41_dev, SENSOR_CHAN_HUMIDITY, &hum_val);

            /* Print the data */
            printf("CO2: %d ppm, Temp: %d.%d C, Humidity: %d.%d %%RH\n",
                   co2_val.val1, 
                   temp_val.val1, temp_val.val2,
                   hum_val.val1, hum_val.val2);
        }

        k_sleep(K_SECONDS(5)); /* Default periodic measurement interval is 5 seconds */
    }
}
