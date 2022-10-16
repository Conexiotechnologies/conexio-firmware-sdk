/*---------------------------------------------------------------------------*/
/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2022 Conexio Technologies, Inc
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*---------------------------------------------------------------------------*/
#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>
#include <random/rand32.h>
#include <net/mqtt.h>
#include <nrf_modem.h>
#include <net/socket.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/modem_info.h>
#include <logging/log.h>
#if defined(CONFIG_MODEM_KEY_MGMT)
#include <modem/modem_key_mgmt.h>
#endif
#include <dk_buttons_and_leds.h>
#include "certificates.h"
#include "env_sensors.h"
#include <math.h>
#include <stdlib.h>
/*---------------------------------------------------------------------------*/
LOG_MODULE_REGISTER(mqtt_app, CONFIG_MQTT_DATACAKE_LOG_LEVEL);

K_SEM_DEFINE(lte_connected, 0, 1);

static struct k_work_delayable cloud_update_work;

/* Define the length of the IMEI AT COMMAND response buffer */
#define CGSN_RESP_LEN 19
#define IMEI_LEN 20
uint8_t client_id[IMEI_LEN];

/* Stratus button state data. Not pressed on default */
char button_state_data[] = "0";

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
	} else {
		LOG_ERR("Publishing of app_version_info PASSED");
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
/**@brief Send client ID information to Datacake. */
static void client_id_send(void)
{
	int err;
	err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_IMEI,
					client_id,
					strlen(client_id));

	if (err) {
		LOG_ERR("Publishing of client_id failed: %d", err);
	}
}
/*---------------------------------------------------------------------------*/
/* Request battery voltage data from the modem and publish to cloud */
static void modem_battery_voltage_send(void)
{
	int err;
	int16_t bat_voltage = 0;
	char buf[CONFIG_MQTT_PAYLOAD_BUFFER_SIZE] = {0};
	err = modem_info_short_get(MODEM_INFO_BATTERY, &bat_voltage);

	if (err != sizeof(bat_voltage)) {
		LOG_ERR("modem_info_short_get, error: %d", err);
	}
	else 
	{
		LOG_INF("modem battery: %u volts", bat_voltage);
	}

	/* Composes a string formatting for the data */
	snprintf(buf, sizeof(buf),"%u", bat_voltage);

	err = data_publish(&client,
					MQTT_QOS_1_AT_LEAST_ONCE,
					CONFIG_MQTT_PUB_TOPIC_BAT,
					buf,
					strlen(buf)); //since MQTT publishes string format

	if (err) {
		LOG_ERR("Publishing of batt volt failed: %d", err);
	}
}
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

	modem_battery_voltage_send();
	 
	return;
error:
	LOG_ERR("sensor_data_send failed: %d", err);
}
#endif /* CONFIG_ENVIRONMENT_SENSORS */
/*---------------------------------------------------------------------------*/
/* Periodically Update the cloud with the device vitals */
static void cloud_update_work_fn(struct k_work *work)
{
	LOG_INF("Sending device vitals to cloud at intervals of %d sec", 
			CONFIG_CLOUD_MESSAGE_PUBLICATION_INTERVAL);

	modem_battery_voltage_send();
	modem_rsrp_data_send();
	button_state_info_send();

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
#endif /* CONFIG_ENVIRONMENT_SENSORS */
}
/*---------------------------------------------------------------------------*/
/* Publish device data on button press */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & BIT(CONFIG_BUTTON_EVENT_BTN_NUM - 1)) {
		button_state_data[0] = '1';
		
		app_version_info_send();
		client_id_send();
		env_data_send();

		k_work_schedule(&cloud_update_work, K_NO_WAIT);
	}
	else {
		button_state_data[0] = '0';
	}
}
/*---------------------------------------------------------------------------*/
#if defined(CONFIG_MQTT_LIB_TLS)
static int certificates_provision(void)
{
	int err = 0;

	LOG_INF("Provisioning certificates");

#if defined(CONFIG_NRF_MODEM_LIB) && defined(CONFIG_MODEM_KEY_MGMT)

	err = modem_key_mgmt_write(CONFIG_MQTT_TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   CA_CERTIFICATE,
				   strlen(CA_CERTIFICATE));
	if (err) {
		LOG_ERR("Failed to provision CA certificate: %d", err);
		return err;
	}

#endif

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
	LOG_INF("%s%s", log_strdup(prefix), log_strdup(buf));
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
			LOG_INF("IPv4 Address found %s", log_strdup(ipv4_addr));

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
/**@brief Get the device IMEI number/ID */
static const uint8_t* client_id_get(void)
{
	enum at_cmd_state at_state;
	char imei_buf[CGSN_RESP_LEN];
	int err;

	/* Initialize AT CMD driver */
	err = at_cmd_init();
	if (err) {
		LOG_ERR("at_cmd failed to initialize, error: %d", err);
	}
    /* Write AT command, read and store the IMEI number */
	err = at_cmd_write("AT+CGSN", imei_buf, sizeof(imei_buf), &at_state);

	if (err) {
		LOG_ERR("Error when trying to do at_cmd_write: %d, at_state: %d",
			err, at_state);
	}
	snprintf(client_id, sizeof(client_id), "%s", imei_buf);

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
	LOG_INF("client_id: %s", log_strdup(client_id_get()));

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
	rsrp_value_latest = rsrp_value;

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
	/* Take into account the RSRP offset value */
	rsrp_current = rsrp_value_latest - MODEM_INFO_RSRP_OFFSET_VAL;

	LOG_INF("RSRP: %d", rsrp_current);
    
	/* If last sent data to cloud is same as current, skip publishing */
	if (rsrp_current == rsrp_prev) {
		return;
	}
	
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
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f\n",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			printk("%s\n", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle\n");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
		       evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}
/*---------------------------------------------------------------------------*/
static int configure_low_power(void)
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
/*---------------------------------------------------------------------------*/
static void modem_init(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		err = lte_lc_init();
		if (err) {
			printk("Modem initialization failed, error: %d\n", err);
			return;
		}
	}
}
/*---------------------------------------------------------------------------*/
static void modem_connect(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		err = lte_lc_connect_async(lte_handler);
		if (err) {
			printk("Connecting to LTE network failed, error: %d\n",
			       err);
			return;
		}
	}
}
/*---------------------------------------------------------------------------*/
#endif

void main(void)
{
	int err;
	uint32_t connect_attempt = 0;

	LOG_INF("Stratus MQTT Datacake sample started, version: %s", CONFIG_APP_VERSION);

	k_work_queue_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       CONFIG_APPLICATION_WORKQUEUE_PRIORITY, NULL);

#if defined(CONFIG_MQTT_LIB_TLS)
	err = certificates_provision();
	if (err != 0) {
		LOG_ERR("Failed to provision certificates");
		return;
	}
#endif /* defined(CONFIG_MQTT_LIB_TLS) */

#if defined(CONFIG_NRF_MODEM_LIB)

	/* Initialize the modem before calling configure_low_power(). This is
	 * because the enabling of RAI is dependent on the
	 * configured network mode which is set during modem initialization.
	 */
	modem_init();

	err = configure_low_power();
	if (err) {
		printk("Unable to set low power configuration, error: %d\n", err);
	}

	modem_connect();

	k_sem_take(&lte_connected, K_FOREVER);
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
		return;
	}
    
	work_init();
	dk_buttons_init(button_handler);
	sensors_init();

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
		return;
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
