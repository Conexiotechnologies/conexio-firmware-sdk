/*--------------------------------------------------------------------------*/
/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2022 IoT Creators
 * Copyright (c) 2023 Conexio Technologies, Inc
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/*--------------------------------------------------------------------------*/
#include <zephyr.h>
#include <stdio.h>
#include <modem/lte_lc.h> 		
#include <nrf_modem_at.h>		
#include <modem/modem_info.h> 	
#include <net/socket.h>			
#include <device.h> 			
#include <drivers/sensor.h> 	
#include <drivers/sensor/sht4x.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
/*--------------------------------------------------------------------------*/
LOG_MODULE_REGISTER(cloud_client, CONFIG_UDP_SAMPLE_LOG_LEVEL);
/*--------------------------------------------------------------------------*/
#define MODEM_APN "scs.telekom.tma.iot"
#define TEMP_CALIBRATION_OFFSET 3
#define RECV_BUF_SIZE 			2048
#define RCV_POLL_TIMEOUT_MS 	1000 /* Milliseconds */
/*--------------------------------------------------------------------------*/
/* Sensor data */
static struct sensor_value temp, hum;
const struct device *sht;

#if !DT_HAS_COMPAT_STATUS_OKAY(sensirion_sht4x)
#error "No sensirion,sht4x compatible node found in the device tree"
#endif
/*--------------------------------------------------------------------------*/
/* The devicetree node identifier for the Blue LED "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led_0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN		DT_GPIO_PIN(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN		0
#endif
/*--------------------------------------------------------------------------*/
/* UDP Socket */
static int client_fd;
static bool socket_open = false;
static struct sockaddr_storage host_addr;

/* Workqueues */
static struct k_work_delayable server_transmission_work;
static struct k_work_delayable data_fetch_work;
static struct k_work_delayable poll_data_work;

K_SEM_DEFINE(lte_connected, 0, 1);
struct modem_param_info modem_param;

static char recv_buf[RECV_BUF_SIZE];
/*--------------------------------------------------------------------------*/
static void fetch_sensor_data(const struct device *sensor)
{
	if (sensor_sample_fetch(sensor)) {
		LOG_ERR("Failed to fetch sample from SHT4X device");
		return;
	}

	sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &hum);
}
/*--------------------------------------------------------------------------*/
static void transmit_udp_data(char *data, size_t len)
{
	int err;
	if (data != NULL){
		LOG_INF("Sending UDP payload, length: %u, data: %s", len, data);
		err = send(client_fd, data, len, 0);
		if (err < 0) {
			LOG_ERR("Failed to transmit UDP packet, %d", errno);
		}
	}
}
/*--------------------------------------------------------------------------*/
int receive_udp_data(char *buf, int buf_size)
{
	int bytes;
	bytes = recv(client_fd, buf, buf_size, 0);
	if (bytes < 0) {
		LOG_ERR("recv() failed, err %d", errno);
	}
	else if (bytes > 0) {
		// Make sure buf is NULL terminated (for safe use)
		if (bytes < buf_size) {
			buf[bytes] = '\0';
		} else {
			buf[buf_size - 1] = '\0';
		}
		LOG_INF("Received UDP data, length: %u, data: %s", bytes, buf);
		return bytes;
	}
	return 0;
}
/*--------------------------------------------------------------------------*/
/* Event Handler - used when data received via UDP */
static void udp_evt_handler(char *buf)
{
	LOG_INF("Handling UDP data, data: %s", buf);

	if (strcmp(buf, "led-on") == 0) {
		/* Turns Blue LED ON */
		gpio_pin_set(led_0.port, PIN, 1);
	}
	else if (strcmp(buf, "led-off") == 0){
		/* Turns Blue LED OFF */
		gpio_pin_set(led_0.port, PIN, 0);
	}
	else
		LOG_ERR("Received invalid command/string");
}
/*--------------------------------------------------------------------------*/
static void data_fetch_work_fn(struct k_work *work)
{
	/* Update sensor + modem data */
	fetch_sensor_data(sht);
	modem_info_params_get(&modem_param);

	/* Reschedule work task */
	k_work_schedule(&data_fetch_work, K_SECONDS(CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS));
}
/*--------------------------------------------------------------------------*/
static void initial_data_transmission(void)
{
	/* Get current modem parameters */
	modem_info_params_get(&modem_param);
	char data_output[128];
	char imei[16];
	char operator[8];
	modem_info_string_get(MODEM_INFO_IMEI, imei, sizeof(imei));
	modem_info_string_get(MODEM_INFO_OPERATOR, operator, sizeof(operator));

	/* Format network data to JSON */
	sprintf(data_output, "{\"Msg\":\"Event: Stratus connected, %s, %s, %d\",\"Oper\":\"%s\",\"Bd\":%d}",
		imei, operator, modem_param.network.current_band.value, operator, modem_param.network.current_band.value);
	/* Transmit data via UDP Socket */
	LOG_INF("Transmitting initial UDP data");
	transmit_udp_data(data_output, strlen(data_output));
}
/*--------------------------------------------------------------------------*/
static void server_transmission_work_fn(struct k_work *work)
{
	/* Format sensor data to JSON */
	char data_output[128];

	/* Format sensor data to JSON */
	sprintf(data_output,"{\"Temp\":%d.%02d,\"Humid\":%d.%02d}",
		temp.val1, temp.val2, hum.val1, hum.val2);

	/* Transmit data via UDP Socket */
	LOG_INF("Transmitting sensor data to IoTCreators cloud");
	transmit_udp_data(data_output, strlen(data_output));

	/* Reschedule work task */
	k_work_schedule(&server_transmission_work, K_SECONDS(CONFIG_UDP_DATA_UPLOAD_FREQUENCY_SECONDS));
}
/*--------------------------------------------------------------------------*/
static void poll_data_work_fn(struct k_work *work)
{
	if (socket_open){
		struct pollfd fds[1];
		int ret = 0;

		fds[0].fd = client_fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		ret = poll(fds, 1, RCV_POLL_TIMEOUT_MS);
		if (ret > 0) {
			int bytes;
			bytes = receive_udp_data(recv_buf, RECV_BUF_SIZE);
			if (bytes > 0) {
				udp_evt_handler(recv_buf);
			}
		}
	}
	/* Reschedule work task for RXing data */
	k_work_schedule(&poll_data_work, K_SECONDS(CONFIG_UDP_DATA_DOWNLOAD_FREQUENCY_SECONDS));
}
/*--------------------------------------------------------------------------*/
static void work_init(void)
{
	k_work_init_delayable(&server_transmission_work, server_transmission_work_fn);
	k_work_init_delayable(&data_fetch_work, data_fetch_work_fn);
	k_work_init_delayable(&poll_data_work, poll_data_work_fn);
}
/*--------------------------------------------------------------------------*/
#if defined(CONFIG_NRF_MODEM_LIB)
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			socket_open=false;
			break;
		}
		LOG_INF("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		LOG_INF("PSM parameter update: TAU: %d, Active time: %d",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			LOG_INF("%s", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_INF("LTE cell changed: Cell ID: %d, Tracking area: %d",
		       evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}
/*--------------------------------------------------------------------------*/
static int configure_low_power(void)
{
	int err;

#if defined(CONFIG_UDP_PSM_ENABLE)
	/* LTE Power Saving Mode */
	err = lte_lc_psm_req(true);
	if (err) {
		LOG_ERR("lte_lc_psm_req, error: %d", err);
	}
#else
	err = lte_lc_psm_req(false);
	if (err) {
		LOG_ERR("lte_lc_psm_req, error: %d", err);
	}
#endif

#if defined(CONFIG_UDP_EDRX_ENABLE)
	/* Enhanced Discontinuous Reception */
	err = lte_lc_edrx_req(true);
	if (err) {
		LOG_ERR("lte_lc_edrx_req, error: %d", err);
	}
#else
	err = lte_lc_edrx_req(false);
	if (err) {
		LOG_ERR("lte_lc_edrx_req, error: %d", err);
	}
#endif

#if defined(CONFIG_UDP_RAI_ENABLE)
	/** Release Assistance Indication */
	err = lte_lc_rai_req(true);
	if (err) {
		LOG_ERR("lte_lc_rai_req, error: %d", err);
	}
#endif

	return err;
}
/*--------------------------------------------------------------------------*/
static void modem_init(void)
{
	int err;
	char response[128];

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		err = lte_lc_init();
		if (err) {
			LOG_ERR("Modem initialization failed, error: %d", err);
			return;
		}
		
		/* Fetch Modem IMEI */
		LOG_INF("Reading Modem IMEI");
		err = nrf_modem_at_cmd(response, sizeof(response), "AT+CGSN=%d", 1);
		if (err) {
			LOG_ERR("Read Modem IMEI failed, err %d", err);
			return;
		}else{
			LOG_INF("Modem IMEI: %s", response);
		}

		/* Setup APN for the PDP Context */
		LOG_INF("Setting up the APN");
		char *apn_stat = "AT%XAPNSTATUS=1,\"" MODEM_APN "\"";
		char *at_cgdcont = "AT+CGDCONT=0,\"IPV4V6\",\"" MODEM_APN "\"";
		nrf_modem_at_printf(apn_stat); // allow use of APN
		err = nrf_modem_at_printf(at_cgdcont); // use conf. APN for PDP context 0 (default LTE bearer)
		if (err) {
			LOG_ERR("AT+CGDCONT set cmd failed, err %d", err);
			return;
		}
		err = nrf_modem_at_cmd(response, sizeof(response), "AT+CGDCONT?", NULL);
		if (err) {
			LOG_ERR("APN check failed, err %d", err);
			return;
		}else{
			LOG_INF("AT+CGDCONT?: %s", response);
		}
		/* Init modem info module */
		modem_info_init();
		modem_info_params_init(&modem_param);
	}
}
/*--------------------------------------------------------------------------*/
static void modem_connect(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		err = lte_lc_connect_async(lte_handler);
		if (err) {
			LOG_ERR("Connecting to LTE network failed, error: %d", err);
			return;
		}
	}
}
#endif
/*--------------------------------------------------------------------------*/
static void server_disconnect(void)
{
	(void)close(client_fd);
}
/*--------------------------------------------------------------------------*/
static int server_init(void)
{
	struct sockaddr_in *server4 = ((struct sockaddr_in *)&host_addr);

	server4->sin_family = AF_INET;
	server4->sin_port = htons(CONFIG_UDP_SERVER_PORT);

	inet_pton(AF_INET, CONFIG_UDP_SERVER_ADDRESS_STATIC,
		  &server4->sin_addr);

	return 0;
}
/*--------------------------------------------------------------------------*/
static int server_connect(void)
{
	int err;

	client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {
		LOG_ERR("Failed to create UDP socket: %d", errno);
		err = -errno;
		goto error;
	}

	err = connect(client_fd, (struct sockaddr *)&host_addr,
		      sizeof(struct sockaddr_in));
	if (err < 0) {
		LOG_ERR("Connect failed : %d", errno);
		goto error;
	}
	socket_open = true;
	return 0;

error:
	server_disconnect();

	return err;
}
/*--------------------------------------------------------------------------*/
void main(void)
{
	int err;
	int ret_0;

	LOG_INF("Conexio Stratus < > IoT Creators Demo Application");
	printk("Dev kit: %s\n", CONFIG_BOARD);
    
	/* Setup sht4x sensor */
	sht = DEVICE_DT_GET_ANY(sensirion_sht4x);

	if (!device_is_ready(sht)) {
		LOG_ERR("Device %s is not ready.", sht->name);
		return;
	}

	if (!device_is_ready(led_0.port)) {
		return;
	}

	ret_0 = gpio_pin_configure_dt(&led_0, GPIO_OUTPUT_ACTIVE);
	if (ret_0 < 0) {
		return;
	}
    
	/* Turns Blue LED ON */
	gpio_pin_set(led_0.port, PIN, 1);
    	k_msleep(1000);
	/* Turns Blue LED OFF */
	gpio_pin_set(led_0.port, PIN, 0);

	/* Init routines */
	work_init();

	/* Initialize the modem before calling configure_low_power(). This is
	 * because the enabling of RAI is dependent on the
	 * configured network mode which is set during modem initialization.
	 */
	modem_init();
	err = configure_low_power();
	if (err) {
		LOG_ERR("Unable to set low power configuration, error: %d",err);
	}

	modem_connect();
	k_sem_take(&lte_connected, K_FOREVER);
	k_msleep(500);

	/* Init UDP Socket */
	err = server_init();
	if (err) {
		LOG_ERR("Not able to initialize UDP server connection");
		return;
	}

	/* Connect to UDP Socket */
	err = server_connect();
	if (err) {
		LOG_ERR("Not able to connect to UDP server");
		return;
	}

	/* Perform initial data transmission & schedule periodic tasks */
	initial_data_transmission();
	k_work_schedule(&data_fetch_work, K_NO_WAIT);
	k_work_schedule(&server_transmission_work, K_SECONDS(5));
	k_work_schedule(&poll_data_work, K_SECONDS(3));	
}
/*--------------------------------------------------------------------------*/
