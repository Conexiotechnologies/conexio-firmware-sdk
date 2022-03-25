/*
 * Copyright (c) 2021 Golioth, Inc.
 * Copyright (c) 2022 Conexio Technologies, Inc
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

/*
 * This function sends SHT4X data to lightdb stream path `/environment`.
 */
static void sensor_value_set(uint32_t tempDec, uint32_t tempFloat, uint32_t humDec, uint32_t humFloat)
{
	char sbuf[60];
	int err;

	snprintk(sbuf, sizeof(sbuf) - 1, "{\"temp\":%d.%06d,\"humidity\":%d.%06d}",
		      tempDec, tempFloat, humDec, humFloat);

	err = golioth_lightdb_set(client,
				  GOLIOTH_LIGHTDB_STREAM_PATH("environment"),
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  sbuf, strlen(sbuf));
	if (err) {
		LOG_WRN("Failed to send sensor data: %d", err);
	}
}

void main(void)
{
	LOG_INF("Start Stratus <> Golioth Light DB sensor stream sample");

	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	
	if (!device_is_ready(sht)) {
		LOG_ERR("Device %s is not ready.\n", sht->name);
		return;
	}

	golioth_system_client_start();

	while (true) {
		
		/* Fetch environmental data from SHT4X sensor */
		fetch_sensor_data(sht);

		LOG_INF("temp: %d.%06d; humidity: %d.%06d",
			temp.val1, temp.val2, hum.val1, hum.val2);
		
		sensor_value_set(temp.val1, temp.val2, hum.val1, hum.val2);

        /* Sleep for X seconds before sampling again */
		k_sleep(SLEEP_TIME_SEC);
	}
}
