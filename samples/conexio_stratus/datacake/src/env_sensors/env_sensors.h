/*
 * Copyright (c) 2024 Conexio Technologies, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ENV_SENSORS_H_
#define ENV_SENSORS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file env_sensors.h
 * @defgroup env_sensors ENV_SENSORS Environmental sensors interface¨
 * @{
 * @brief Module for interfacing environmental sensors for asset tracker
 *
 * @details Basic basic for interfacing environmental sensors for the
 *          asset tracker application.
 *          Supported sensor types are Temperature and Pressure sensors.
 *
 */

#include <zephyr/types.h>


/** @brief Environmental sensor types supported */
typedef enum {
	/** Temperature sensor. */
	ENV_SENSOR_TEMPERATURE,
	/** The Humidity sensor. */
	ENV_SENSOR_HUMIDITY,
} env_sensor_t;

typedef struct {
	/** Sensor type. */
	env_sensor_t type;
	/** Sensor sample value. */
	double value;
	/** Sensor sample uptime. */
	int64_t ts;
} env_sensor_data_t;

typedef void (*env_sensors_data_ready_cb)(void);

/**
 * @brief Get latest sampled temperature data.
 *
 * @param sensor_data Pointer to memory to store latest temperature data.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_get_temperature(env_sensor_data_t *sensor_data);

/**
 * @brief Get latest sampled humidity data.
 *
 * @param sensor_data Pointer to memory to store latest humidity data.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_get_humidity(env_sensor_data_t *sensor_data);

/**
 * @brief Initialize and start the environmental sensors.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_init_and_start(struct k_work_q *work_q,
			       const env_sensors_data_ready_cb cb);

/**
 * @brief Set environmental sensor's poll/send interval.
 *
 * @param interval_s Interval, in seconds. 0 to disable.
 *
 */
void env_sensors_set_send_interval(const uint32_t interval_s);

/**
 * @brief Get environmental sensor's poll/send interval.
 *
 * @return Interval, in seconds.
 */
uint32_t env_sensors_get_send_interval(void);

/**
 * @brief Enable or disable back-off delay for sending environmental data.
 *
 * @param backoff_enable True to enable back-off delay, otherwise false.
 *
 */
void env_sensors_set_backoff_enable(const bool backoff_enable);

/**
 * @brief Perform an immediate poll of the environmental sensor.
 *
 * @return 0 if the operation was successful, otherwise a (negative) error code.
 */
int env_sensors_poll(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ENV_SENSORS_H_ */
