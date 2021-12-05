/*
 * Copyright (c) 2021 Golioth, Inc.
 * Copyright (c) 2021 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_lightdb_stream, LOG_LEVEL_DBG);

#include <net/coap.h>
#include <net/golioth/system_client.h>
#include <net/golioth/wifi.h>

#include <drivers/sensor.h>
#include <drivers/sensor/sht4x.h>

#include <stdlib.h>
#include <stdio.h>

#define SLEEP_TIME_SEC		K_SECONDS(10)

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static struct sensor_value temp, hum;

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

static void fetch_sensor_data(const struct device *sensor)
{
	if (sensor_sample_fetch(sensor)) {
		LOG_ERR("Failed to fetch sample from SHT4X device\n");
		return;
	}

	sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &hum);
}

void main(void)
{
	char str_temperature[32];
	char str_humidity[32];

	LOG_DBG("Start Stratus <> Golioth Light DB sensor stream sample");

	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	
	int err;

	if (!device_is_ready(sht)) {
		LOG_ERR("Device %s is not ready.\n", sht->name);
		return;
	}

	golioth_system_client_start();

	while (true) {
		
		/* Fetch environmental data from SHT4X sensor */
		fetch_sensor_data(sht);

		snprintk(str_temperature, sizeof(str_temperature), "%d.%06d", temp.val1, abs(temp.val2));
		str_temperature[sizeof(str_temperature) - 1] = '\0';

		LOG_DBG("Sending temperature data %s", log_strdup(str_temperature));

		err = golioth_lightdb_set(client,
					  GOLIOTH_LIGHTDB_STREAM_PATH("temp"),
					  COAP_CONTENT_FORMAT_TEXT_PLAIN,
					  str_temperature,
					  strlen(str_temperature));
		if (err) {
			LOG_WRN("Failed to send temperature data: %d", err);
		}

        /* Let previous data transaction to complete */
		k_sleep(K_SECONDS(1));

		snprintk(str_humidity, sizeof(str_humidity), "%d.%06d", hum.val1, abs(hum.val2));
		str_humidity[sizeof(str_humidity) - 1] = '\0';

		LOG_DBG("Sending humidity data %s", log_strdup(str_humidity));

		err = golioth_lightdb_set(client,
					  GOLIOTH_LIGHTDB_STREAM_PATH("humidity"),
					  COAP_CONTENT_FORMAT_TEXT_PLAIN,
					  str_humidity,
					  strlen(str_humidity));
		if (err) {
			LOG_WRN("Failed to send humidity data: %d", err);
		}
        /* Sleep for X seconds before sampling again */
		k_sleep(SLEEP_TIME_SEC);
	}
}
