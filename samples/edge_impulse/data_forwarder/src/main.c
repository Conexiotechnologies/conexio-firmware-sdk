/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <device.h>
#include <drivers/sensor.h>
#include <drivers/uart.h>

#define SAMPLE_PERIOD_MS	100

#define UART_LABEL			DT_LABEL(DT_NODELABEL(uart0))
#define UART_BUF_SIZE		64

static const struct device *sensor;
static const struct device *uart_dev;
static atomic_t uart_busy;

static void uart_cb(const struct device *dev, struct uart_event *evt,
		    void *user_data)
{
	if (evt->type == UART_TX_DONE) {
		atomic_cas(&uart_busy, true, false);
	}
}

static int init(void)
{
	sensor = device_get_binding(DT_LABEL(DT_INST(0, st_lis2dh)));

	if (!sensor) {
		printk("Cannot bind %s sensor\n", DT_LABEL(DT_INST(0, st_lis2dh)));
		return -ENXIO;
	}

	uart_dev = device_get_binding(UART_LABEL);

	if (!uart_dev) {
		printk("Cannot bind UART\n");
		return -ENXIO;
	}

	int err = uart_callback_set(uart_dev, uart_cb, NULL);

	if (err) {
		printk("Cannot set UART callback (err %d)\n", err);
	}

	return err;
}

static int provide_sensor_data(void)
{
	struct sensor_value accel[3];
	const char *overrun = "";
	int err = sensor_sample_fetch(sensor);

	if (err == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		err = 0;
	}
	/* Sample LIS2DH sensor. */
	if (err == 0) {
		err = sensor_channel_get(sensor,
					SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}

	/* Send data over UART. */
	static uint8_t buf[UART_BUF_SIZE];

	if (!atomic_cas(&uart_busy, false, true)) {
		printk("UART not ready\n");
		printk("Please use lower sampling frequency\n");
		return -EBUSY;
	}

	int res = snprintf(buf, sizeof(buf), "%.2f,%.2f,%.2f\r\n",
			   sensor_value_to_double(&accel[0]),
			   sensor_value_to_double(&accel[1]),
			   sensor_value_to_double(&accel[2]));

	if (res < 0) {
		printk("snprintf returned error (err %d)\n", res);
		return res;
	} else if (res >= sizeof(buf)) {
		printk("UART_BUF_SIZE is too small to store the data\n");
		printk("%d bytes are required\n", res);
		return -ENOMEM;
	}

	err = uart_tx(uart_dev, buf, res, SYS_FOREVER_MS);

	if (err) {
		printk("Cannot send data over UART (err %d)\n", err);
	} else {
		printk("Sent data: %s", buf);
	}

	return err;
}

void main(void)
{
	if (init()) {
		return;
	}
	printk("Conexio Stratus to Edge Impulse data forwarder initialized\n");

	int64_t uptime = k_uptime_get();
	int64_t next_timeout = uptime + SAMPLE_PERIOD_MS;

	while (1) {
		if (provide_sensor_data()) {
			break;
		}

		uptime = k_uptime_get();

		if (next_timeout <= uptime) {
			printk("Sampling frequency is too high for sensor\n");
			break;
		}

		k_sleep(K_MSEC(next_timeout - uptime));
		next_timeout += SAMPLE_PERIOD_MS;
	}
}
