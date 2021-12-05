/*
 * Copyright (c) 2021 Golioth, Inc.
 * Copyright (c) 2021 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_dfu, LOG_LEVEL_DBG);

#include <net/coap.h>
#include <net/golioth/fw.h>
#include <net/golioth/system_client.h>

#include <logging/log_ctrl.h>
#include <power/reboot.h>
#include <stdlib.h>
#include <stdio.h>

#include <drivers/sensor.h>
#include <drivers/sensor/sht4x.h>

#include "flash.h"

#define REBOOT_DELAY_SEC	1
#define SLEEP_TIME_SEC		K_SECONDS(1000)

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

static struct coap_reply coap_replies[4];

struct dfu_ctx {
	struct golioth_fw_download_ctx fw_ctx;
	struct flash_img_context flash;
	char version[65];
};

static struct dfu_ctx update_ctx;
static enum golioth_dfu_result dfu_initial_result = GOLIOTH_DFU_RESULT_INITIAL;

static struct sensor_value temp, hum;

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif

static int data_received(struct golioth_blockwise_download_ctx *ctx,
			 const uint8_t *data, size_t offset, size_t len,
			 bool last)
{
	struct dfu_ctx *dfu = CONTAINER_OF(ctx, struct dfu_ctx, fw_ctx);
	int err;

	LOG_DBG("Received %zu bytes at offset %zu%s", len, offset,
		last ? " (last)" : "");

	if (offset == 0) {
		err = flash_img_prepare(&dfu->flash);
		if (err) {
			return err;
		}
	}

	err = flash_img_buffered_write(&dfu->flash, data, len, last);
	if (err) {
		LOG_ERR("Failed to write to flash: %d", err);
		return err;
	}

	if (offset > 0 && last) {
		err = golioth_fw_report_state(client, "main",
					      current_version_str,
					      dfu->version,
					      GOLIOTH_FW_STATE_DOWNLOADED,
					      GOLIOTH_DFU_RESULT_INITIAL);
		if (err) {
			LOG_ERR("Failed to update to '%s' state: %d", "downloaded", err);
		}

		err = golioth_fw_report_state(client, "main",
					      current_version_str,
					      dfu->version,
					      GOLIOTH_FW_STATE_UPDATING,
					      GOLIOTH_DFU_RESULT_INITIAL);
		if (err) {
			LOG_ERR("Failed to update to '%s' state: %d", "updating", err);
		}

		LOG_INF("Requesting upgrade");

		err = boot_request_upgrade(BOOT_UPGRADE_TEST);
		if (err) {
			LOG_ERR("Failed to request upgrade: %d", err);
			return err;
		}

		LOG_INF("Rebooting in %d second(s)", REBOOT_DELAY_SEC);

		/* Synchronize logs */
		LOG_PANIC();

		k_sleep(K_SECONDS(REBOOT_DELAY_SEC));

		sys_reboot(SYS_REBOOT_COLD);
	}

	return 0;
}

static uint8_t *uri_strip_leading_slash(uint8_t *uri, size_t *uri_len)
{
	if (*uri_len > 0 && uri[0] == '/') {
		(*uri_len)--;
		return &uri[1];
	}

	return uri;
}

static int golioth_desired_update(const struct coap_packet *update,
				  struct coap_reply *reply,
				  const struct sockaddr *from)
{
	struct dfu_ctx *dfu = &update_ctx;
	struct coap_reply *fw_reply;
	const uint8_t *payload;
	uint16_t payload_len;
	size_t version_len = sizeof(dfu->version) - 1;
	uint8_t uri[64];
	uint8_t *uri_p;
	size_t uri_len = sizeof(uri);
	int err;

	payload = coap_packet_get_payload(update, &payload_len);
	if (!payload) {
		LOG_ERR("No payload in CoAP!");
		return -EIO;
	}

	LOG_HEXDUMP_DBG(payload, payload_len, "Desired");

	err = golioth_fw_desired_parse(payload, payload_len,
				       dfu->version, &version_len,
				       uri, &uri_len);
	if (err) {
		LOG_ERR("Failed to parse desired version: %d", err);
		return err;
	}

	dfu->version[version_len] = '\0';

	if (version_len == strlen(current_version_str) &&
	    !strncmp(current_version_str, dfu->version, version_len)) {
		LOG_INF("Desired version (%s) matches current firmware version!",
			log_strdup(current_version_str));
		return -EALREADY;
	}

	fw_reply = coap_reply_next_unused(coap_replies, ARRAY_SIZE(coap_replies));
	if (!reply) {
		LOG_ERR("No more reply handlers");
		return -ENOMEM;
	}

	uri_p = uri_strip_leading_slash(uri, &uri_len);

	err = golioth_fw_report_state(client, "main",
				      current_version_str,
				      dfu->version,
				      GOLIOTH_FW_STATE_DOWNLOADING,
				      GOLIOTH_DFU_RESULT_INITIAL);
	if (err) {
		LOG_ERR("Failed to update to '%s' state: %d", "downloading", err);
	}

	err = golioth_fw_download(client, &dfu->fw_ctx, uri_p, uri_len,
				  fw_reply, data_received);
	if (err) {
		LOG_ERR("Failed to request firmware: %d", err);
		return err;
	}

	return 0;
}

static void golioth_on_connect(struct golioth_client *client)
{
	struct coap_reply *reply;
	int err;
	int i;

	err = golioth_fw_report_state(client, "main",
				      current_version_str,
				      NULL,
				      GOLIOTH_FW_STATE_IDLE,
				      dfu_initial_result);
	if (err) {
		LOG_ERR("Failed to report firmware state: %d", err);
	}

	for (i = 0; i < ARRAY_SIZE(coap_replies); i++) {
		coap_reply_clear(&coap_replies[i]);
	}

	reply = coap_reply_next_unused(coap_replies, ARRAY_SIZE(coap_replies));
	if (!reply) {
		LOG_ERR("No more reply handlers");
	}

	err = golioth_fw_observe_desired(client, reply, golioth_desired_update);
	if (err) {
		coap_reply_clear(reply);
	}
}

static void golioth_on_message(struct golioth_client *client,
			       struct coap_packet *rx)
{
	uint16_t payload_len;
	const uint8_t *payload;
	uint8_t type;

	type = coap_header_get_type(rx);
	payload = coap_packet_get_payload(rx, &payload_len);

	(void)coap_response_received(rx, NULL, coap_replies,
				     ARRAY_SIZE(coap_replies));
}

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
	int err;
	char str_temperature[32];
	char str_humidity[32];

	LOG_DBG("Starting Stratus + Golioth DFU sample");

	const struct device *sht = DEVICE_DT_GET_ANY(sensirion_sht4x);

	if (!device_is_ready(sht)) {
		LOG_ERR("Device %s is not ready.\n", sht->name);
		return;
	}

	if (!boot_is_img_confirmed()) {
		/*
		 * There is no shared context between previous update request
		 * and current boot, so treat current image 'confirmed' flag as
		 * an indication whether previous update process was successful
		 * or not.
		 */
		dfu_initial_result = GOLIOTH_DFU_RESULT_FIRMWARE_UPDATED_SUCCESSFULLY;

		err = boot_write_img_confirmed();
		if (err) {
			LOG_ERR("Failed to confirm image: %d", err);
		}
	}

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
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
