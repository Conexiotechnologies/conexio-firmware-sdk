/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <ncs_version.h>

LOG_MODULE_REGISTER(device_test, LOG_LEVEL_INF);

/* Response buffer - large enough for multi-line AT responses */
#define AT_RESPONSE_BUF_SIZE 512

/* Delay between AT commands to give the modem time to respond */
#define AT_CMD_DELAY_MS 200

static char at_buf[AT_RESPONSE_BUF_SIZE];

#define GPIO_NODE	DT_NODELABEL(gpio0)

/* The devicetree node identifier for the "led0" alias. */
#define LED DT_ALIAS(led0)

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED, gpios);
static const struct device *gpio_dev;
/*---------------------------------------------------------------------------*/
/* Set the sleep time */
#define SLEEP_TIME_MS 1000

void init_led(void)
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Send an AT command and log the response.
 *
 * @param label   Human-readable label printed before the response.
 * @param cmd     AT command string (printf-style, no trailing \r\n needed).
 * @return 0 on success, negative errno on modem error.
 */
static int at_cmd_send(const char *label, const char *cmd)
{
	int err;

	memset(at_buf, 0, sizeof(at_buf));
	err = nrf_modem_at_cmd(at_buf, sizeof(at_buf), "%s", cmd);

	/* Strip trailing newlines for cleaner log output */
	size_t len = strlen(at_buf);
	while (len > 0 && (at_buf[len - 1] == '\r' || at_buf[len - 1] == '\n')) {
		at_buf[--len] = '\0';
	}

	if (err < 0) {
		LOG_ERR("[%s] AT cmd failed (err %d): %s", label, err, at_buf);
	} else {
		LOG_INF("[%s] %s", label, at_buf);
	}

	k_sleep(K_MSEC(AT_CMD_DELAY_MS));
	return err;
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Identify the modem hardware and firmware.
 */
static void test_modem_identity(void)
{
	LOG_INF("======================================\n");
	LOG_INF("=== Modem Identity ===");

	/* Manufacturer identification */
	at_cmd_send("Manufacturer",    "AT+CGMI");

	/* Model identification */
	at_cmd_send("Model",           "AT+CGMM");

	/* Modem firmware revision */
	at_cmd_send("MFW Revision",     "AT+CGMR");

	/* IMEI */
	at_cmd_send("IMEI",            "AT+CGSN=1");

	LOG_INF("======================================\n");
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Read modem temperature and device UUID.
 */
static void test_modem_diagnostics(void)
{
	LOG_INF("=== Modem Diagnostics ===");

	/* Modem die temperature */
	at_cmd_send("Modem Temp (%XTEMP?)",  "AT%XTEMP?");

	/* Modem voltage */
	at_cmd_send("Modem Voltage (%XVBAT)", "AT%XVBAT");

	LOG_INF("======================================\n");
}
/*---------------------------------------------------------------------------*/
int main(void)
{	
	int err;

	LOG_INF("=======================================");
	LOG_INF("	Hello from %s", CONFIG_BOARD		 );
	LOG_INF("  	NCS v" NCS_VERSION_STRING "        	");
	LOG_INF("=======================================\n");

	/* Initialize the modem library */
	err = nrf_modem_lib_init();
	if (err < 0) {
		LOG_ERR("Modem library init failed (err %d)\n", err);
		return 0;
	}
	LOG_INF("Modem library initialized\n");

	gpio_dev = DEVICE_DT_GET(GPIO_NODE);
	if (!gpio_dev) {
		printk("Error getting GPIO device binding\r\n");
		return false;
	}

	init_led();
    
	/* --- Run a few test sequence --- */
	test_modem_identity();
	test_modem_diagnostics();

	while (1) {
		// Blink the LED on Stratus board
		gpio_pin_toggle_dt(&led);
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
/*---------------------------------------------------------------------------*/