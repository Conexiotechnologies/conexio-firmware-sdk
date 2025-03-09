/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include <zephyr/drivers/sensor/sht4x.h>
#include <zephyr/drivers/gpio.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

#define SENSOR_PWR DT_ALIAS(power)
static struct gpio_dt_spec sensor_power = GPIO_DT_SPEC_GET(SENSOR_PWR, gpios);

/**
 * @brief Set power gate pin state
 *
 * @param state bool state ON or OFF
 * @return none
 */
static void set_power_gate(bool state){
	gpio_pin_set_dt(&sensor_power, state);
	if(state) {
		k_msleep(1000); //give 1 sec delay
	}
}

int main(void)
{
	printf("SHT4X sensor sample running on %s\n", CONFIG_BOARD);

	// Configure power pin as output and low
	gpio_pin_configure_dt(&sensor_power, GPIO_OUTPUT_INACTIVE);

	// setup sht4x sensor 
	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	struct sensor_value temp, hum;

	// Turn ON power to the sensor
	set_power_gate(true);

	if (!device_is_ready(sht)) {
		printf("Device %s is not ready.\n", sht->name);
		return 0;
	}

	// Turn OFF power to the sensor
	set_power_gate(false);

	while (true) {
		// Turn ON power to the sensor to fetch a sample
		set_power_gate(true);

		if (sensor_sample_fetch(sht)) {
			printf("Failed to fetch sample from SHT4X device\n");
			return 0;
		}

		sensor_channel_get(sht, SENSOR_CHAN_AMBIENT_TEMP, &temp);
		sensor_channel_get(sht, SENSOR_CHAN_HUMIDITY, &hum);

		// Turn OFF power to the sensor
		set_power_gate(false);

		printf("SHT4X: Temp: %d.%06d degC; RH: %d.%06d %%\n", temp.val1, temp.val2, hum.val1, hum.val2);

		k_sleep(K_MSEC(3000));
	}
}
