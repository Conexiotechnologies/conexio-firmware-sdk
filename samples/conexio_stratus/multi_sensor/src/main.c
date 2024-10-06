/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#if defined(CONFIG_SHT4X)
#include <zephyr/drivers/sensor/sht4x.h>
#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif
#endif /* CONFIG_SHT4X */

#define GPIO_NODE	DT_NODELABEL(gpio0)

/* The devicetree node identifier for the "led0" alias. */
#define RED_LED DT_ALIAS(led0)

#define LED_OFF 0
#define LED_ON !LED_OFF

static struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(RED_LED, gpios);
static const struct device *gpio_dev;

void init_led(void)
{
	gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
}

void blink_led(void)
{
	// Blink the white LED on Stratus Pro nRF9151/nRF9161 board
	// Red on Stratus nRF9160 board
	gpio_pin_set_dt(&red_led, LED_ON);
	k_msleep(500);
	gpio_pin_set_dt(&red_led, LED_OFF);
	k_msleep(500);
}

#if defined(CONFIG_SHT4X)
static void fetch_and_display_sht4x(const struct device *sensor)
{
	struct sensor_value temp, hum;
	if (sensor_sample_fetch(sensor)) {
		printf("Failed to fetch sample from SHT4X device\n");
	}

	sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &hum);
	printf("SHT4X: Temp: %d.%06d degC; RH: %d.%06d %%\n", temp.val1, temp.val2, hum.val1, hum.val2);
}
#endif /* CONFIG_SHT4X */

static void fetch_and_display_accel(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[3];
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
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
		printf("#%u, uptime: %u ms, x %f, y: %f, z: %f\n",
		       count, k_uptime_get_32(),
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));
	}
}

int main(void)
{
	printf("Conexio Stratus multi sensor example\n");
	//LED setup
	gpio_dev = DEVICE_DT_GET(GPIO_NODE);

	if (!gpio_dev) {
		printk("Error getting LED GPIO device binding\r\n");
		return 0;
	}

	init_led();

	//accelerometer setup
	const struct device *lis2dh_sensor = DEVICE_DT_GET_ANY(st_lis2dh);

	if (lis2dh_sensor == NULL) {
		printf("No device found\n");
		return 0;
	}
	if (!device_is_ready(lis2dh_sensor)) {
		printf("Device %s is not ready\n", lis2dh_sensor->name);
		return 0;
	}

#if defined(CONFIG_SHT4X)
	//sht4x setup
	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	
	if (!device_is_ready(sht)) {
		printf("Device %s is not ready.\n", sht->name);
		return 0;
	}
#endif /* CONFIG_SHT4X */

	while (true) {
		blink_led();
		fetch_and_display_accel(lis2dh_sensor);
		k_sleep(K_MSEC(500));

#if defined(CONFIG_SHT4X)
		fetch_and_display_sht4x(sht);
		k_sleep(K_MSEC(2000));
#endif /* CONFIG_SHT4X */

	}
}
