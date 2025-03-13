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
#include <zephyr/drivers/mfd/npm1300.h>
#include <zephyr/drivers/regulator.h>

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

static const struct device *buck2 = DEVICE_DT_GET(DT_NODELABEL(reg_3v3));

/* Set the sleep time */
#define SLEEP_TIME_MS 5000

static int setup_npm1300(void)
{
  int err;

  /* Get nPM1300 pmic */
  static const struct device *pmic = DEVICE_DT_GET(DT_INST(0, nordic_npm1300));
  if (!pmic)
  {
    printf("Failed to get PMIC device\n");
    return -ENODEV;
  }

  /* Disable 3.3V BUCK2 if not already disabled */
  if (regulator_is_enabled(buck2))
  {
    err = regulator_disable(buck2);
    if (err < 0)
    {
      printf("Failed to disable buck2: %d", err);
      return err;
    }
  }

  return 0;
}
/**
 * @brief Sets 3V3 regulator ON/OFF for QWIIC, LED, and GPS LNA
 *
 * @param state bool state ON or OFF
 * @return none
 */
static int set_power_gate(bool state)
{
    int err = 0;

    if (!buck2) {
        printf("Buck2 regulator device not found\n");
        return -ENODEV;
    }

    if (state) {
        if (!regulator_is_enabled(buck2)) {
            err = regulator_enable(buck2);
            if (err < 0) {
                printf("Failed to enable buck2: %d\n", err);
                return err;
            }
            k_msleep(500);
        }
    } else {
        if (regulator_is_enabled(buck2)) {
            err = regulator_disable(buck2);
            if (err < 0) {
                printf("Failed to disable buck2: %d\n", err);
                return err;
            }
        }
    }

    return err;
}

int main(void)
{
	printf("SHT4X sensor sample running on %s\n", CONFIG_BOARD);
	setup_npm1300();

	// setup sht4x sensor 
	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	struct sensor_value temp, hum;

	// Turn ON power to the sensor connected to QWIIC port
	set_power_gate(true);

	if (!device_is_ready(sht)) {
		printf("Device %s is not ready.\n", sht->name);
		return 0;
	}

	// Turn OFF power to the sensor connected to QWIIC port
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

		// Turn OFF power to the sensor connected to QWIIC port
		set_power_gate(false);

		printf("SHT4X: Temp: %d.%06d degC; RH: %d.%06d %%\n", temp.val1, temp.val2, hum.val1, hum.val2);

		k_msleep(SLEEP_TIME_MS);

	}
}
