/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <dk_buttons_and_leds.h>

/* Include the header file for the GNSS interface */
#include <nrf_modem_gnss.h>

/* Define the PVT data frame variable */
static struct nrf_modem_gnss_pvt_data_frame pvt_data;

/* Declare helper variables to find the TTFF */
static int64_t gnss_start_time;
static bool first_fix = false;

static K_SEM_DEFINE(lte_connected, 0, 1);

LOG_MODULE_REGISTER(GNSS_sample, LOG_LEVEL_INF);


static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modem library");

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize the modem library, error: %d", err);
		return err;
	}

	err = lte_lc_init();
	if (err) {
		LOG_ERR("Failed to initialize LTE Link Controller, error: %d", err);
		return err;
	}

	return 0;
}

/* Define a function to log fix data in a readable format */
static void print_fix_data(struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	LOG_INF("Latitude:       %.06f", pvt_data->latitude);
	LOG_INF("Longitude:      %.06f", pvt_data->longitude);
	LOG_INF("Altitude:       %.01f m", pvt_data->altitude);
	LOG_INF("Time (UTC):     %02u:%02u:%02u.%03u",
	       pvt_data->datetime.hour,
	       pvt_data->datetime.minute,
	       pvt_data->datetime.seconds,
	       pvt_data->datetime.ms);
}


static void gnss_event_handler(int event)
{
	int err;

	switch (event) {
	/* On a PVT event, confirm if PVT data is a valid fix */
	case NRF_MODEM_GNSS_EVT_PVT:
		LOG_INF("Searching...");
		/* Print satellite information */
		int num_satellites = 0;
		for (int i = 0; i < 12 ; i++) {
			if (pvt_data.sv[i].signal != 0) {
				LOG_INF("sv: %d, cn0: %d", pvt_data.sv[i].sv, pvt_data.sv[i].cn0);
				num_satellites++;
			}
		}
		LOG_INF("Number of current satellites: %d", num_satellites);
		err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
		if (err) {
			LOG_ERR("nrf_modem_gnss_read failed, err %d", err);
			return;
		}
		if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
			dk_set_led_on(DK_LED1);
			print_fix_data(&pvt_data);
			/* Print the time to first fix */
			if (!first_fix) {
				LOG_INF("Time to first fix: %2.1lld s", (k_uptime_get() - gnss_start_time)/1000);
				first_fix = true;
			}
			return;
		}
		break;
	/* Log when the GNSS sleeps and wakes up */
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_INF("GNSS has woken up");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_INF("GNSS enter sleep after fix");
		break;
	default:
		break;
	}
}

int main(void)
{
	int err;
	
	LOG_INF("Stratus GNSS sample started");

	if (dk_leds_init() != 0) {
		LOG_ERR("Failed to initialize the LEDs Library");
	}

	err = modem_configure();
	if (err) {
		LOG_ERR("Failed to configure the modem");
		return 0;
	}

	/* Activate only the GNSS stack */
	if (lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS) != 0) {
		LOG_ERR("Failed to activate GNSS functional mode");
		return 0;
	}

	/* Register the GNSS event handler */
	if (nrf_modem_gnss_event_handler_set(gnss_event_handler) != 0) {
		LOG_ERR("Failed to set GNSS event handler");
		return 0;
	}

	/* Set the GNSS fix interval and GNSS fix retry period */
	if (nrf_modem_gnss_fix_interval_set(CONFIG_GNSS_PERIODIC_INTERVAL) != 0) {
		LOG_ERR("Failed to set GNSS fix interval");
		return 0;
	}

	if (nrf_modem_gnss_fix_retry_set(CONFIG_GNSS_PERIODIC_TIMEOUT) != 0) {
		LOG_ERR("Failed to set GNSS fix retry");
		return 0;
	}

	/* Start the GNSS receiver */
	LOG_INF("Starting GNSS");
	if (nrf_modem_gnss_start() != 0) {
		LOG_ERR("Failed to start GNSS");
		return 0;
	}

	/* Log the current system uptime */
	gnss_start_time = k_uptime_get();

	return 0;
}
