/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2022 Conexio Technologies, Inc.
 * 
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * 
 * \author Rajeev Piyare <rajeev@conexiotech.com>
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <drivers/sensor.h>
#include <spinlock.h>
#include "env_sensors.h"
#include <devicetree.h>
#include <device.h>
#include <drivers/sensor/sht4x.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(env_sensors, CONFIG_MQTT_DATACAKE_LOG_LEVEL);

const struct device *sht4x_dev;

#define ENV_INIT_DELAY_S (5) /* Polling delay upon initialization */
#define MAX_INTERVAL_S   (INT_MAX/MSEC_PER_SEC)

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

static struct k_work_q *env_sensors_work_q;

struct env_sensor {
	env_sensor_data_t sensor;
	enum sensor_channel channel;
	struct k_spinlock lock;
};

static struct env_sensor temp_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_TEMPERATURE,
		.value = 0,
	},
	.channel = SENSOR_CHAN_AMBIENT_TEMP,
};

static struct env_sensor humid_sensor = {
	.sensor =  {
		.type = ENV_SENSOR_HUMIDITY,
		.value = 0,
	},
	.channel = SENSOR_CHAN_HUMIDITY,
};

/* Array containg environment sensors available on the board. */
static struct env_sensor *env_sensors[] = {
	&temp_sensor,
	&humid_sensor
};

static struct k_work_delayable env_sensors_poller;
static env_sensors_data_ready_cb data_ready_cb;
static uint32_t data_send_interval_s = CONFIG_ENVIRONMENT_DATA_SEND_INTERVAL;
static bool backoff_enabled;
static bool initialized;

static inline int submit_poll_work(const uint32_t delay_s)
{
	return k_work_reschedule_for_queue(env_sensors_work_q,
					      &env_sensors_poller,
					      K_SECONDS((uint32_t)delay_s));
}

int env_sensors_poll(void)
{
	return initialized ? submit_poll_work(0) : -ENXIO;
}

static void env_sensors_poll_fn(struct k_work *work)
{
	int num_sensors = ARRAY_SIZE(env_sensors);
	struct sensor_value data[num_sensors];
	int err;

	if (data_send_interval_s == 0) {
		return;
	}

	if (IS_ENABLED(CONFIG_SHT4X)) {
		err = sensor_sample_fetch(sht4x_dev);
		if (err) {
			LOG_ERR("Failed to fetch data from sht4x error: %d", err);
		}
	}

	for (int i = 0; i < num_sensors; i++) {
		if (!(IS_ENABLED(CONFIG_SHT4X))) {
			err = sensor_sample_fetch_chan(sht4x_dev,
				env_sensors[i]->channel);
			if (err) {
				LOG_ERR("Failed to fetch data from sht4x error: %d",err);
			}
		}
		err = sensor_channel_get(sht4x_dev,
			env_sensors[i]->channel, &data[i]);
		if (err) {
			LOG_ERR("Failed to fetch data from sht4x error: %d", err);
		} else {
			k_spinlock_key_t key = k_spin_lock(&(env_sensors[i]->lock));

			env_sensors[i]->sensor.value = sensor_value_to_double(&data[i]);
			env_sensors[i]->sensor.ts = k_uptime_get();
			k_spin_unlock(&(env_sensors[i]->lock), key);
		}
	}

	if (data_ready_cb) {
		data_ready_cb();
	}

	submit_poll_work(data_send_interval_s);
}

/**@brief Initialize environment sensors. */
int env_sensors_init_and_start(struct k_work_q *work_q,
			       const env_sensors_data_ready_cb cb)
{
	if ((work_q == NULL) || (cb == NULL)) {
		return -EINVAL;
	}

	sht4x_dev = DEVICE_DT_GET_ANY(sensirion_sht4x);

	if (!device_is_ready(sht4x_dev)) {
		LOG_ERR("Device %s is not ready.\n", sht4x_dev->name);
		return -ENODEV;
	} 

	env_sensors_work_q = work_q;

	data_ready_cb = cb;

	k_work_init_delayable(&env_sensors_poller, env_sensors_poll_fn);

	initialized = true;

	LOG_INF("Environmental sensors initialized");

	return (data_send_interval_s > 0) ?
		submit_poll_work(ENV_INIT_DELAY_S) : 0;
}

int env_sensors_get_temperature(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&temp_sensor.lock);

	memcpy(sensor_data, &(temp_sensor.sensor), sizeof(temp_sensor.sensor));
	k_spin_unlock(&temp_sensor.lock, key);
	return 0;
}

int env_sensors_get_humidity(env_sensor_data_t *sensor_data)
{
	if (sensor_data == NULL) {
		return -1;
	}
	k_spinlock_key_t key = k_spin_lock(&humid_sensor.lock);

	memcpy(sensor_data, &(humid_sensor.sensor),
		sizeof(humid_sensor.sensor));
	k_spin_unlock(&humid_sensor.lock, key);
	return 0;
}

void env_sensors_set_send_interval(const uint32_t interval_s)
{
	if (interval_s == data_send_interval_s) {
		return;
	}

	data_send_interval_s = MIN(interval_s, MAX_INTERVAL_S);

	if (!initialized) {
		return;
	}

	if (data_send_interval_s) {
		/* restart work for new interval to take effect */
		env_sensors_poll();
	} else {
		k_work_cancel_delayable(&env_sensors_poller);
	}
}

uint32_t env_sensors_get_send_interval(void)
{
	return data_send_interval_s;
}

void env_sensors_set_backoff_enable(const bool enable)
{
	backoff_enabled = enable;
}
