/*---------------------------------------------------------------------------*/
/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*---------------------------------------------------------------------------*/
#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/uart.h>
#include <string.h>
#include <zephyr/random/random.h>
#include <zephyr/net/mqtt.h>
#include <helpers/nrfx_reset_reason.h>
#include <nrf_modem.h>
#include <zephyr/net/socket.h>
#include <modem/lte_lc.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_MODEM_KEY_MGMT)
#include <modem/modem_key_mgmt.h>
#endif
#include <modem/nrf_modem_lib.h>
#include <dk_buttons_and_leds.h>
#include "certificate.h"
#include "env_sensors.h"

#if defined(CONFIG_NRF_FUEL_GAUGE)
#include <zephyr/drivers/mfd/npm1300.h>
#include "fuel_gauge.h"
#endif

#if defined(CONFIG_MOTION_SENSOR)
#include "motion_sensor.h"
#endif

#include <math.h>
#include <stdlib.h>
/*---------------------------------------------------------------------------*/
LOG_MODULE_REGISTER(mqtt_app, CONFIG_MQTT_DATACAKE_LOG_LEVEL);

K_SEM_DEFINE(lte_connected, 0, 1);

static struct k_work_delayable cloud_update_work;

#if defined(CONFIG_NRF_FUEL_GAUGE)
static const struct device *pmic = DEVICE_DT_GET(DT_INST(0, nordic_npm1300));
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(pmic_charger));
static volatile bool vbus_connected;
#endif

/*** Device ID ***/
#define IMEI_LEN 15
#define CGSN_RESPONSE_LENGTH (IMEI_LEN + 6 + 1) /* Add 6 for \r\nOK\r\n and 1 for \0 */
#define CLIENT_ID_LEN 20

/* Stratus button state data. Not pressed on default */
char button_state_data[] = "0";
char reset_reason_str[128];

/* Buffers for MQTT client. */
static uint8_t rx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[CONFIG_MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE];

/* The mqtt client struct */
static struct mqtt_client client;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

/* File descriptor */
static struct pollfd fds;

/* Struct that holds data from the modem information module. */
static struct modem_param_info modem_param;
/* Value that always holds the latest RSRP value. */
int32_t rsrp_value_latest;

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(application_stack_area, CONFIG_APPLICATION_WORKQUEUE_STACK_SIZE);
static struct k_work_q application_work_q;
/*---------------------------------------------------------------------------*/
#if defined(CONFIG_NRF_FUEL_GAUGE)
static void event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(NPM1300_EVENT_VBUS_DETECTED)) {
		printk("Vbus connected\n");
		vbus_connected = true;
	}

	if (pins & BIT(NPM1300_EVENT_VBUS_REMOVED)) {
		printk("Vbus removed\n");
		vbus_connected = false;
	}
}
#endif
/*---------------------------------------------------------------------------*/
/* Forward declaration of functions */
#if defined(CONFIG_ENVIRONMENT_SENSORS)
static void env_data_send(void);
#endif
static void sensors_init(void);

static int data_publish(struct mqtt_client *c, enum mqtt_qos qos, 
			char *pub_topic, uint8_t *data, size_t len);

#if CONFIG_MODEM_INFO
static void modem_rsrp_data_send(void);
#endif /* CONFIG_MODEM_INFO */
/*---------------------------------------------------------------------------*/
static void reset_reason_str_get(char *str, uint32_t reason)
{
	size_t len;

	*str = '\0';

	if (reason & NRFX_RESET_REASON_RESETPIN_MASK) {
		(void)strcat(str, "PIN reset | ");
	}
	if (reason & NRFX_RESET_REASON_DOG_MASK) {
		(void)strcat(str, "watchdog | ");
	}
	if (reason & NRFX_RESET_REASON_OFF_MASK) {
		(void)strcat(str, "wakeup from power-off | ");
	}
	if (reason & NRFX_RESET_REASON_DIF_MASK) {
		(void)strcat(str, "debug interface wakeup | ");
	}
	if (reason & NRFX_RESET_REASON_SREQ_MASK) {
		(void)strcat(str, "software | ");
	}
	if (reason & NRFX_RESET_REASON_LOCKUP_MASK) {
		(void)strcat(str, "CPU lockup | ");
	}
	if (reason & NRFX_RESET_REASON_CTRLAP_MASK) {
		(void)strcat(str, "control access port | ");
	}

	len = strlen(str);
	if (len == 0) {
		(void)strcpy(str, "power-on reset");
	} else {
		str[len - 3] = '\0';
	}
}
/*---------------------------------------------------------------------------*/
static void print_reset_reason(void)
{
	uint32_t reset_reason;

	/* Read RESETREAS register value and clear current reset reason(s). */
	reset_reason = nrfx_reset_reason_get();
	nrfx_reset_reason_clear(reset_reason);

	reset_reason_str_get(reset_reason_str, reset_reason);

	LOG_ERR("\nReset reason: %s\n", reset_reason_str);
}
/*---------------------------------------------------------------------------*/
static void last_reset_reason_info_send(void)
{
	int err;
	err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_RESETRSN,
					reset_reason_str,
					sizeof(reset_reason_str)-1);

	if (err) {
		LOG_ERR("Publishing of last reset_reason_info failed: %d", err);
	} 
}
/*---------------------------------------------------------------------------*/
#if defined(CONFIG_APP_VERSION)
/**@brief Send application version information to Datacake. */
static void app_version_info_send(void)
{
	int err;
	err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_VER,
					CONFIG_APP_VERSION,
					sizeof(CONFIG_APP_VERSION)-1);

	if (err) {
		LOG_ERR("Publishing of app_version_info failed: %d", err);
	} 
}
#endif /* CONFIG_APP_VERSION */
/*---------------------------------------------------------------------------*/
/* Publish the Stratus button state */
static void button_state_info_send(void)
{
	int err;
	err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_BUTTON,
					button_state_data,
					sizeof(button_state_data)-1);
	if (err) {
		LOG_ERR("Publishing of button_state_data failed: %d", err);
	}	
}
/*---------------------------------------------------------------------------*/
/* Request battery voltage data and publish to cloud */
#if defined(CONFIG_NRF_FUEL_GAUGE)
void battery_vitals_send(void)
{
	int err;
	float bat_voltage = 0;
	float avg_I_mA = 0;
	float SoC = 0;
	float tte = 0;
	float ttf = 0;

	char buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE] = {0};

	/* Update fuel gauge samples before publishing data */
	fuel_gauge_update(charger, vbus_connected);

	bat_voltage = pmic_data.voltage;
	avg_I_mA = pmic_data.current * 1000;
	SoC = pmic_data.soc;
	tte = pmic_data.tte;
	ttf = pmic_data.ttf;

	if (bat_voltage != 0) {
		LOG_INF("Device battery: %.3f V", (double)bat_voltage);
		/* Composes a string formatting for the data */
		snprintf(buf, sizeof(buf),"%.3f", (double)bat_voltage);
		err = data_publish(&client,
						MQTT_QOS_1_AT_LEAST_ONCE,
						CONFIG_MQTT_PUB_TOPIC_BAT,
						buf,
						strlen(buf));
		if (err) {
			LOG_ERR("Publishing of batt volt data failed: %d", err);
		}
	}

	if (avg_I_mA != 0) {
		LOG_INF("Avg current: %.3f mA", (double)avg_I_mA);
		snprintf(buf, sizeof(buf),"%.3f", (double)avg_I_mA);
		err = data_publish(&client,
						MQTT_QOS_1_AT_LEAST_ONCE,
						CONFIG_MQTT_PUB_TOPIC_CURRENT,
						buf,
						strlen(buf));
		if (err) {
			LOG_ERR("Publishing of current cons data failed: %d", err);
		}
	}

	if (SoC != 0) {
		LOG_INF("SoC: %.2f %%", (double)SoC);
		snprintf(buf, sizeof(buf),"%.2f", (double)SoC);
		err = data_publish(&client,
						MQTT_QOS_1_AT_LEAST_ONCE,
						CONFIG_MQTT_PUB_TOPIC_SOC,
						buf,
						strlen(buf));
		if (err) {
			LOG_ERR("Publishing of SoC data failed: %d", err);
		}
	}

	if (tte > 0) {
		LOG_INF("TTE: %.0f sec", (double)tte);
		snprintf(buf, sizeof(buf),"%.0f", (double)tte);
		err = data_publish(&client,
						MQTT_QOS_1_AT_LEAST_ONCE,
						CONFIG_MQTT_PUB_TOPIC_TTE,
						buf,
						strlen(buf));
		if (err) {
			LOG_ERR("Publishing of TTE data failed: %d", err);
		}
	}

    if (ttf > 0) {
		LOG_INF("TTF: %.0f sec", (double)ttf);
		snprintf(buf, sizeof(buf),"%.0f", (double)ttf);
		err = data_publish(&client,
						MQTT_QOS_1_AT_LEAST_ONCE,
						CONFIG_MQTT_PUB_TOPIC_TTF,
						buf,
						strlen(buf));
		if (err) {
			LOG_ERR("Publishing of TTF data failed: %d", err);
		}
	}
}
#endif /* CONFIG_NRF_FUEL_GAUGE */
/*---------------------------------------------------------------------------*/
#if defined(CONFIG_ENVIRONMENT_SENSORS)
/**@brief Get environment data from sensors and send to cloud. */
static void env_data_send(void)
{
	int err;
	env_sensor_data_t env_data;
	char buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE] = {0};

	env_sensors_set_backoff_enable(false);

	if (env_sensors_get_temperature(&env_data) == 0) {
		/* Sanity check to make sure data is actually from temperature sensor */
		if(env_data.type == ENV_SENSOR_TEMPERATURE){

			snprintk(buf, sizeof(buf),"%.2f", env_data.value);
			err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_TEMP,
					buf,
					strlen(buf));

			if (err) {
				goto error;
			}
		}	
	}

	if (env_sensors_get_humidity(&env_data) == 0) {
		/* Sanity check to make sure data is actually from humidity sensor */
		if(env_data.type == ENV_SENSOR_HUMIDITY){
			snprintk(buf, sizeof(buf),"%.2f", env_data.value);
			err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_HUM,
					buf,
					strlen(buf));

			if (err) {
				goto error;
			}
		}
	}

#if defined(CONFIG_NRF_FUEL_GAUGE)
	battery_vitals_send();
#endif
	 
	return;
error:
	LOG_ERR("sensor_data_send failed: %d", err);
}
#endif /* CONFIG_ENVIRONMENT_SENSORS */
/*---------------------------------------------------------------------------*/
#if defined(CONFIG_MOTION_SENSOR)
/**@brief Get motion data from sensors and send to cloud. */
static void motion_data_send(void)
{
	int err;
	char buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE] = {0};

	/* Create motion data event */
	struct motion_data accel_data;

	err = motion_sensor_sample_fetch(&accel_data);
	
	if (err) {
			LOG_ERR("Unable to get motion sample: Err: %i", err);
	}
    else {
		LOG_INF("Motion detected: x: %f y: %f z: %f", 
			sensor_value_to_double(&accel_data.x), 
			sensor_value_to_double(&accel_data.y), 
			sensor_value_to_double(&accel_data.z));

		//pack and send x-axis data
		snprintk(buf, sizeof(buf),"%f", sensor_value_to_double(&accel_data.x));
		err = data_publish(&client,
				MQTT_QOS_1_AT_LEAST_ONCE,
				CONFIG_MQTT_PUB_TOPIC_MOTION_X,
				buf,
				strlen(buf));

		if (err) {
			goto error;
		}

		//pack and send y-axis data
		snprintk(buf, sizeof(buf),"%f", sensor_value_to_double(&accel_data.y));
		err = data_publish(&client,
				MQTT_QOS_1_AT_LEAST_ONCE,
				CONFIG_MQTT_PUB_TOPIC_MOTION_Y,
				buf,
				strlen(buf));

		if (err) {
			goto error;
		}

		//pack and send z-axis data
		snprintk(buf, sizeof(buf),"%f", sensor_value_to_double(&accel_data.z));
		err = data_publish(&client,
				MQTT_QOS_1_AT_LEAST_ONCE,
				CONFIG_MQTT_PUB_TOPIC_MOTION_Z,
				buf,
				strlen(buf));

		if (err) {
			goto error;
		}
	}

error:
	LOG_ERR("motion_data_send failed: %d", err);
}
#endif /* CONFIG_MOTION_SENSOR */
/*---------------------------------------------------------------------------*/
/* Periodically Update the cloud with the device vitals */
static void cloud_update_work_fn(struct k_work *work)
{
	LOG_INF("Sending device vitals to cloud at intervals of %d sec", 
			CONFIG_CLOUD_MESSAGE_PUBLICATION_INTERVAL);

#if defined(CONFIG_NRF_FUEL_GAUGE)
	battery_vitals_send();
#endif

	modem_rsrp_data_send();

#if defined(CONFIG_MOTION_SENSOR)
	motion_data_send();
#endif

	k_work_schedule(
			&cloud_update_work,
			K_SECONDS(CONFIG_CLOUD_MESSAGE_PUBLICATION_INTERVAL));
}
/*---------------------------------------------------------------------------*/
static void work_init(void)
{
	k_work_init_delayable(&cloud_update_work, cloud_update_work_fn);
}
/*---------------------------------------------------------------------------*/
/**@brief Initializes the sensors that are used by the application. */
static void sensors_init(void)
{
#if defined(CONFIG_ENVIRONMENT_SENSORS)
	env_sensors_init_and_start(&application_work_q, env_data_send);
	LOG_INF("Sending environmental data to cloud at intervals of %d sec", 
			CONFIG_ENVIRONMENT_DATA_SEND_INTERVAL);
#endif /* CONFIG_ENVIRONMENT_SENSORS */

#if defined(CONFIG_MOTION_SENSOR)
	motion_sensor_init();
#endif /* CONFIG_MOTION_SENSOR */
}
/*---------------------------------------------------------------------------*/
/* Publish device data on button press */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(CONFIG_BUTTON_EVENT_BTN_NUM - 1)) {
		button_state_data[0] = '1';
		
		last_reset_reason_info_send();
		app_version_info_send();
		modem_rsrp_data_send();

#if defined(CONFIG_ENVIRONMENT_SENSORS)
		env_data_send();
#endif /* CONFIG_ENVIRONMENT_SENSORS */

#if defined(CONFIG_MOTION_SENSOR)
		motion_data_send();
#endif /* CONFIG_MOTION_SENSOR */

		button_state_info_send();
		k_work_schedule(&cloud_update_work, K_NO_WAIT);
	}
	else {
		button_state_data[0] = '0';
	}
}
/*---------------------------------------------------------------------------*/
#if defined(CONFIG_MQTT_LIB_TLS)
static int certificate_provision(void)
{
	int err = 0;
	bool exists;

	err = modem_key_mgmt_exists(CONFIG_MQTT_TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists);
	if (err) {
		LOG_ERR("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		/* Let's compare the existing credential */
		err = modem_key_mgmt_cmp(CONFIG_MQTT_TLS_SEC_TAG,
					 MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					 CA_CERTIFICATE, 
					 strlen(CA_CERTIFICATE));
		LOG_INF("%s\n", err ? "mismatch" : "match");
		if (!err) {
			return 0;
		}
	}
	LOG_INF("Provisioning certificates");
	err = modem_key_mgmt_write(CONFIG_MQTT_TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   CA_CERTIFICATE,
				   strlen(CA_CERTIFICATE));
	if (err) {
		LOG_ERR("Failed to provision CA certificate: %d", err);
		return err;
	}
	return err;
}
#endif /* defined(CONFIG_MQTT_LIB_TLS) */
/*---------------------------------------------------------------------------*/
#if defined(CONFIG_NRF_MODEM_LIB)
/**@brief Recoverable modem library error. */
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	LOG_ERR("Modem library recoverable error: %u", (unsigned int)err);
}
#endif /* defined(CONFIG_NRF_MODEM_LIB) */
/*---------------------------------------------------------------------------*/
/**@brief Function to print strings without null-termination
 */
static void data_print(uint8_t *prefix, uint8_t *data, size_t len)
{
	char buf[len + 1];

	memcpy(buf, data, len);
	buf[len] = 0;
	LOG_INF("%s%s", (char *)prefix, (char *)buf);
}
/*---------------------------------------------------------------------------*/
/**@brief Function to publish data on the configured topic */
static int data_publish(struct mqtt_client *c, enum mqtt_qos qos, char *pub_topic,
	uint8_t *data, size_t len)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = qos;
	param.message.topic.topic.utf8 = (uint8_t *)pub_topic;
	param.message.topic.topic.size = strlen(param.message.topic.topic.utf8);
	param.message.payload.data = data;
	param.message.payload.len = len;
	param.message_id = sys_rand32_get();
	param.dup_flag = 0;
	param.retain_flag = 0;

	data_print("Publishing: ", data, len);
	LOG_INF("to topic: %s len: %u",
		param.message.topic.topic.utf8, param.message.topic.topic.size);

    return mqtt_publish(c, &param);
}
/*---------------------------------------------------------------------------*/
/**@brief Function to subscribe to the configured topic
 */
static int subscribe(void)
{
	struct mqtt_topic subscribe_topic = {
		.topic = {
			.utf8 = CONFIG_MQTT_SUB_TOPIC,
			.size = strlen(CONFIG_MQTT_SUB_TOPIC)
		},
		.qos = MQTT_QOS_1_AT_LEAST_ONCE
	};

	const struct mqtt_subscription_list subscription_list = {
		.list = &subscribe_topic,
		.list_count = 1,
		.message_id = 1234
	};

	LOG_INF("Subscribing to: %s len %u", CONFIG_MQTT_SUB_TOPIC,
		(unsigned int)strlen(CONFIG_MQTT_SUB_TOPIC));

	return mqtt_subscribe(&client, &subscription_list);
}
/*---------------------------------------------------------------------------*/
/**@brief Function to read the published payload */
static int publish_get_payload(struct mqtt_client *c, size_t length)
{
	if (length > sizeof(payload_buf)) {
		return -EMSGSIZE;
	}

	return mqtt_readall_publish_payload(c, payload_buf, length);
}
/*---------------------------------------------------------------------------*/
/**@brief MQTT client event handler */
void mqtt_evt_handler(struct mqtt_client *const c,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed: %d", evt->result);
			break;
		}

		LOG_INF("MQTT client connected");
		subscribe();
		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected: %d", evt->result);
		break;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &evt->param.publish;

		LOG_INF("MQTT PUBLISH result=%d len=%d",
			evt->result, p->message.payload.len);

		err = publish_get_payload(c, p->message.payload.len);

		if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
			const struct mqtt_puback_param ack = {
				.message_id = p->message_id
			};

			/* Send acknowledgment. */
			mqtt_publish_qos1_ack(&client, &ack);
		}

		if (err >= 0) {
			data_print("Received: ", payload_buf,
				p->message.payload.len);
		} else {
			LOG_ERR("publish_get_payload failed: %d", err);
			LOG_INF("Disconnecting MQTT client...");

			err = mqtt_disconnect(c);
			if (err) {
				LOG_ERR("Could not disconnect: %d", err);
			}
		}
	} break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error: %d", evt->result);
			break;
		}

		LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);
		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT SUBACK error: %d", evt->result);
			break;
		}

		LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);
		break;

	case MQTT_EVT_PINGRESP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PINGRESP error: %d", evt->result);
		}
		break;

	default:
		LOG_INF("Unhandled MQTT event type: %d", evt->type);
		break;
	}
}
/*---------------------------------------------------------------------------*/
/**@brief Resolves the configured hostname and
 * initializes the MQTT broker structure
 */
static int broker_init(void)
{
	int err;
	struct addrinfo *result;
	struct addrinfo *addr;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

	err = getaddrinfo(CONFIG_MQTT_BROKER_HOSTNAME, NULL, &hints, &result);
	if (err) {
		LOG_ERR("getaddrinfo failed: %d", err);
		return -ECHILD;
	}

	addr = result;

	/* Look for address of the broker. */
	while (addr != NULL) {
		/* IPv4 Address. */
		if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
			struct sockaddr_in *broker4 =
				((struct sockaddr_in *)&broker);
			char ipv4_addr[NET_IPV4_ADDR_LEN];

			broker4->sin_addr.s_addr =
				((struct sockaddr_in *)addr->ai_addr)
				->sin_addr.s_addr;
			broker4->sin_family = AF_INET;
			broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);

			inet_ntop(AF_INET, &broker4->sin_addr.s_addr,
				  ipv4_addr, sizeof(ipv4_addr));
			LOG_INF("IPv4 Address found %s", ipv4_addr);

			break;
		} else {
			LOG_ERR("ai_addrlen = %u should be %u or %u",
				(unsigned int)addr->ai_addrlen,
				(unsigned int)sizeof(struct sockaddr_in),
				(unsigned int)sizeof(struct sockaddr_in6));
		}

		addr = addr->ai_next;
	}

	/* Free the address. */
	freeaddrinfo(result);

	return err;
}
/*---------------------------------------------------------------------------*/
/* Function to get the client id */
static const uint8_t* client_id_get(void)
{
	static uint8_t client_id[MAX(sizeof(CONFIG_MQTT_CLIENT_ID),
				     CLIENT_ID_LEN)];

	if (strlen(CONFIG_MQTT_CLIENT_ID) > 0) {
		snprintf(client_id, sizeof(client_id), "%s",
			 CONFIG_MQTT_CLIENT_ID);
		goto exit;
	}

	char imei_buf[CGSN_RESPONSE_LENGTH + 1];
	int err;

	err = nrf_modem_at_cmd(imei_buf, sizeof(imei_buf), "AT+CGSN");
	if (err) {
		LOG_ERR("Failed to obtain IMEI, error: %d", err);
		goto exit;
	}

	imei_buf[IMEI_LEN] = '\0';

	snprintf(client_id, sizeof(client_id), "%.*s", IMEI_LEN, imei_buf);

exit:
	LOG_DBG("client_id = %s", (char *)(client_id));

	return client_id;
}
/*---------------------------------------------------------------------------*/
/**@brief Initialize the MQTT client structure */
static int client_init(struct mqtt_client *client)
{
	int err;

	mqtt_client_init(client);

	err = broker_init();
	if (err) {
		LOG_ERR("Failed to initialize broker connection");
		return err;
	}
    
	/* Fetch the client ID which will be stored in client_id buffer*/
	LOG_INF("client_id: %s", client_id_get());

	struct mqtt_utf8 pass, user;
	pass.utf8 = (uint8_t *)CONFIG_MQTT_PASS;
	pass.size = strlen(pass.utf8);

	user.utf8 = (uint8_t *)CONFIG_MQTT_USER;
	user.size = strlen(user.utf8);

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (uint8_t *)CONFIG_MQTT_CLIENT_ID;
	client->client_id.size = strlen(CONFIG_MQTT_CLIENT_ID);
	client->password = &pass;
	client->user_name = &user;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);

	/* MQTT transport configuration */
#if defined(CONFIG_MQTT_LIB_TLS)
	struct mqtt_sec_config *tls_cfg = &(client->transport).tls.config;
	static sec_tag_t sec_tag_list[] = { CONFIG_MQTT_TLS_SEC_TAG };

	LOG_INF("TLS enabled");
	client->transport.type = MQTT_TRANSPORT_SECURE;

	tls_cfg->peer_verify = CONFIG_MQTT_TLS_PEER_VERIFY;
	tls_cfg->cipher_count = 0;
	tls_cfg->cipher_list = NULL;
	tls_cfg->sec_tag_count = ARRAY_SIZE(sec_tag_list);
	tls_cfg->sec_tag_list = sec_tag_list;
	tls_cfg->hostname = CONFIG_MQTT_BROKER_HOSTNAME;

#if defined(CONFIG_NRF_MODEM_LIB)
	tls_cfg->session_cache = IS_ENABLED(CONFIG_MQTT_TLS_SESSION_CACHING) ?
					    TLS_SESSION_CACHE_ENABLED :
					    TLS_SESSION_CACHE_DISABLED;
#else
	/* TLS session caching is not supported by the Zephyr network stack */
	tls_cfg->session_cache = TLS_SESSION_CACHE_DISABLED;

#endif

#else
	client->transport.type = MQTT_TRANSPORT_NON_SECURE;
#endif

	return err;
}
/*---------------------------------------------------------------------------*/
/**@brief Initialize the file descriptor structure used by poll */
static int fds_init(struct mqtt_client *c)
{
	if (c->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds.fd = c->transport.tcp.sock;
	} else {
#if defined(CONFIG_MQTT_LIB_TLS)
		fds.fd = c->transport.tls.sock;
#else
		return -ENOTSUP;
#endif
	}

	fds.events = POLLIN;

	return 0;
}
/*---------------------------------------------------------------------------*/
#if CONFIG_MODEM_INFO
static inline int adjust_rsrp(int input)
{
	if (IS_ENABLED(CONFIG_MODEM_DYNAMIC_DATA_CONVERT_RSRP_TO_DBM)) {
		return RSRP_IDX_TO_DBM(input);
	}

	return input;
}

static void modem_rsrp_handler(char rsrp_value)
{
	/* RSRP raw values that represent actual signal strength are
	 * 0 through 97 (per "nRF91 AT Commands" v1.1).
	 */
	if (rsrp_value > 97) {
		return;
	}
	/* Set temporary variable to hold RSRP value. RSRP callbacks and other
	 * data from the modem info module are retrieved separately.
	 * This temporarily saves the latest value which are sent to
	 * the Data module upon a modem data request.
	 */
	rsrp_value_latest = adjust_rsrp(rsrp_value);

	LOG_DBG("Incoming RSRP status message, RSRP value is %d",
		rsrp_value_latest);
}
/*---------------------------------------------------------------------------*/
/**@brief Publish RSRP data to the cloud. */
static void modem_rsrp_data_send(void)
{
	int err;
	char buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE] = {0};
	static int32_t rsrp_prev; /* RSRP value last sent to cloud */
	int32_t rsrp_current;

	/* The RSRP value is copied locally to avoid any race condition */
	rsrp_current = rsrp_value_latest;

	LOG_INF("Publishing RSRP: %d", rsrp_current);
    
	snprintf(buf, sizeof(buf),"%d", rsrp_current);
	err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_RSRP,
					buf,
					strlen(buf));

	if (err) {
		LOG_ERR("Publishing of RSRP info failed: %d", err);
	}
	rsrp_prev = rsrp_current;
}
/*---------------------------------------------------------------------------*/
static int modem_data_init(void)
{
	int err;

	err = modem_info_init();
	if (err) {
		LOG_INF("modem_info_init, error: %d", err);
		return err;
	}

	err = modem_info_params_init(&modem_param);
	if (err) {
		LOG_INF("modem_info_params_init, error: %d", err);
		return err;
	}

	err = modem_info_rsrp_register(modem_rsrp_handler);
	if (err) {
		LOG_INF("modem_info_rsrp_register, error: %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_MODEM_INFO */

#if defined(CONFIG_NRF_MODEM_LIB)
/*---------------------------------------------------------------------------*/
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		printk("Network registration status: %s\n",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming\n");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		printk("PSM parameter update: TAU: %d, Active time: %d\n",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE:
		printk("eDRX parameter update: eDRX: %.2f s, PTW: %.2f s\n",
		       (double)evt->edrx_cfg.edrx, (double)evt->edrx_cfg.ptw);
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
		       evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle\n");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
		       evt->cell.id, evt->cell.tac);
		break;
	case LTE_LC_EVT_RAI_UPDATE:
		/* RAI notification is supported by modem firmware releases >= 2.0.2 */
		printk("RAI configuration update: "
		       "Cell ID: %d, MCC: %d, MNC: %d, AS-RAI: %d, CP-RAI: %d\n",
		       evt->rai_cfg.cell_id,
		       evt->rai_cfg.mcc,
		       evt->rai_cfg.mnc,
		       evt->rai_cfg.as_rai,
		       evt->rai_cfg.cp_rai);
		break;
	default:
		break;
	}
}
/*---------------------------------------------------------------------------*/
static int configure_lte_low_power(void)
{
	int err;

	/** Power Saving Mode */
	err = lte_lc_psm_req(true);
	if (err) {
		printk("lte_lc_psm_req, error: %d\n", err);
	}

	err = lte_lc_edrx_req(false);
	if (err) {
		printk("lte_lc_edrx_req, error: %d\n", err);
	}

	return err;
}
#endif
/*---------------------------------------------------------------------------*/
static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modem library");

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize the modem library, error: %d", err);
		return err;
	}

	/* Store the certificate in the modem while the modem is in offline mode */
	err = certificate_provision();
	if (err) {
		LOG_ERR("Failed to provision certificates");
		return err;
	}

	LOG_INF("Connecting to LTE network");

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Error in lte_lc_connect_async, error: %d", err);
		return err;
	}

	k_sem_take(&lte_connected, K_FOREVER);
	LOG_INF("Connected to LTE network");

	return 0;
}
/*---------------------------------------------------------------------------*/
int main(void)
{
	int err;
	uint32_t connect_attempt = 0;

#if defined(CONFIG_NRF_FUEL_GAUGE)
	bool fuel_gauge_initialized;
#endif

	LOG_INF("Stratus MQTT Datacake sample started, version: %s", CONFIG_APP_VERSION);

	/* Reset reason can only be read once, because the register needs to be cleared after
	 * reading.
	 */
	print_reset_reason();

	k_work_queue_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       CONFIG_APPLICATION_WORKQUEUE_PRIORITY, NULL);

	err = modem_configure();
	if (err) {
		LOG_ERR("Failed to configure the modem");
		return 0;
	}

#if defined(CONFIG_LTE_LINK_CONTROL)
	lte_lc_register_handler(lte_handler);
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */

#if defined(CONFIG_NRF_MODEM_LIB)

	/* Initialize the modem before calling configure_low_power(). This is
	 * because the enabling of RAI is dependent on the
	 * configured network mode which is set during modem initialization.
	 */

	err = configure_lte_low_power();
	if (err) {
		printk("Unable to set low power configuration, error: %d\n",
		       err);
	}

#endif

#if CONFIG_MODEM_INFO
	err = modem_data_init();
	if (err) {
		LOG_ERR("modem_data_init, error: %d\n", err);
	}
#endif /* CONFIG_MODEM_INFO */

	err = client_init(&client);
	if (err != 0) {
		LOG_ERR("client_init: %d", err);
		return 0;
	}

	dk_buttons_init(button_handler);
	work_init();
	sensors_init();

#if defined(CONFIG_NRF_FUEL_GAUGE)
	static struct gpio_callback event_cb;

	gpio_init_callback(&event_cb, event_callback,
				   BIT(NPM1300_EVENT_VBUS_DETECTED) |
				   BIT(NPM1300_EVENT_VBUS_REMOVED));

	err = mfd_npm1300_add_callback(pmic, &event_cb);
	if (err) {
		printk("Failed to add pmic callback.\n");
		return 0;
	}

	/* Initialise vbus detection status. */
	struct sensor_value val;
	int ret = sensor_attr_get(charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);

	if (ret < 0) {
		return false;
	}

	vbus_connected = (val.val1 != 0) || (val.val2 != 0);

	fuel_gauge_initialized = fuel_gauge_init_and_start();

	if (!fuel_gauge_initialized) {
		LOG_ERR("Fuel gauge init, error: %d", fuel_gauge_initialized);
		return 0;
	}
#endif

do_connect:
	if (connect_attempt++ > 0) {
		LOG_INF("Reconnecting in %d seconds...", CONFIG_MQTT_RECONNECT_DELAY_S);
		k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
	}
	err = mqtt_connect(&client);
	if (err != 0) {
		LOG_ERR("mqtt_connect %d", err);
		goto do_connect;
	}

	err = fds_init(&client);
	if (err != 0) {
		LOG_ERR("fds_init: %d", err);
		return 0;
	}

	while (1) {
		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if (err < 0) {
			LOG_ERR("poll: %d", errno);
			break;
		}

		err = mqtt_live(&client);
		if ((err != 0) && (err != -EAGAIN)) {
			LOG_ERR("ERROR: mqtt_live: %d", err);
			break;
		}

		if ((fds.revents & POLLIN) == POLLIN) {
			err = mqtt_input(&client);
			if (err != 0) {
				LOG_ERR("mqtt_input: %d", err);
				break;
			}
		}

		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR");
			break;
		}

		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			LOG_ERR("POLLNVAL");
			break;
		}
	}

	LOG_INF("Disconnecting MQTT client...");

	err = mqtt_disconnect(&client);
	if (err) {
		LOG_ERR("Could not disconnect MQTT client: %d", err);
	}
	goto do_connect;
}
/*---------------------------------------------------------------------------*/
