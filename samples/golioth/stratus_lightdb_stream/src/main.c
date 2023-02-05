/*--------------------------------------------------------------------------*/
/*
 * Copyright (c) 2021 Golioth, Inc.
 * Copyright (c) 2023 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*--------------------------------------------------------------------------*/
#include <net/golioth/system_client.h>
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sht4x.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_lightdb_stream, LOG_LEVEL_DBG);
/*--------------------------------------------------------------------------*/
#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif
/*--------------------------------------------------------------------------*/
static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static struct sensor_value temp, hum;

static K_SEM_DEFINE(connected, 0, 1);
/*--------------------------------------------------------------------------*/
static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);
}
/*--------------------------------------------------------------------------*/
static void fetch_sensor_data(const struct device *sensor)
{
	if (sensor_sample_fetch(sensor)) {
		LOG_ERR("Failed to fetch sample from SHT4X device\n");
		return;
	}

	sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &hum);
}
/*--------------------------------------------------------------------------*/
static int env_data_push_handler(struct golioth_req_rsp *rsp)
{
	if (rsp->err) {
		LOG_ERR("Failed to push data: %d", rsp->err);
		return rsp->err;
	}

	LOG_DBG("Data successfully pushed to the Golioth Cloud");

	return 0;
}
/*--------------------------------------------------------------------------*/
/*
 * This function sends SHT4X data to lightdb stream path `/environment` 
 * using callback mode (golioth_stream_push_cb).
 */
static void env_data_push_async(uint32_t tempDec, uint32_t tempFloat, uint32_t humDec, uint32_t humFloat)
{
	char sbuf[60];
	int err;
    
	/* Format the environmetal data to JSON */
	snprintk(sbuf, sizeof(sbuf) - 1, "{\"temp\":%d.%06d,\"humidity\":%d.%06d}",
		      tempDec, abs(tempFloat), humDec, abs(humFloat));

	err = golioth_stream_push_cb(client, "environment",
				     GOLIOTH_CONTENT_FORMAT_APP_JSON,
				     sbuf, strlen(sbuf),
				     env_data_push_handler, NULL);
	if (err) {
		LOG_ERR("Failed to push send sensor data: %d", err);
		return;
	}
}
/*--------------------------------------------------------------------------*/
/*
 * This function sends SHT4X data to lightdb stream path `/environment` 
 * using Synchronous mode (golioth_stream_push).
 */
static void env_data_push_sync(uint32_t tempDec, uint32_t tempFloat, uint32_t humDec, uint32_t humFloat)
{
	char sbuf[60];
	int err;
    
	/* Format the environmetal data to JSON */
	snprintk(sbuf, sizeof(sbuf) - 1, "{\"temp\":%d.%06d,\"humidity\":%d.%06d}",
		      tempDec, abs(tempFloat), humDec, abs(humFloat));

	err = golioth_stream_push(client, "environment",
				  GOLIOTH_CONTENT_FORMAT_APP_JSON,
				  sbuf, strlen(sbuf));
	if (err) {
		LOG_ERR("Failed to push send sensor data: %d", err);
		return;
	}

	LOG_INF("Environmental data successfully pushed");
}
/*--------------------------------------------------------------------------*/
void main(void)
{
	LOG_INF("Stratus < > Golioth Light DB sensor stream sample started");

	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);
	
	if (!device_is_ready(sht)) {
		LOG_ERR("Device %s is not ready.\n", sht->name);
		return;
	}

	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
		net_connect();
	}

	client->on_connect = golioth_on_connect;
	golioth_system_client_start();

	k_sem_take(&connected, K_FOREVER);

	while (true) {
		
		/* Fetch latest environmental data from SHT4X sensor */
		fetch_sensor_data(sht);
        
		/* Send data using Synchronous mode */
		LOG_DBG("Sending temp: %d.%06d; humidity: %d.%06d", 
			temp.val1, abs(temp.val2), hum.val1, abs(hum.val2));

		env_data_push_sync(temp.val1, temp.val2, hum.val1, hum.val2);

		k_sleep(K_SECONDS(5));

		/* Send data using Callback-based */
		LOG_DBG("Sending temp: %d.%06d; humidity: %d.%06d", 
			temp.val1, abs(temp.val2), hum.val1, abs(hum.val2));

		env_data_push_async(temp.val1, temp.val2, hum.val1, hum.val2);

		k_sleep(K_SECONDS(5));
	}
}
/*--------------------------------------------------------------------------*/
