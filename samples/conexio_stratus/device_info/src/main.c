/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <nrf_modem.h>
#include <modem/nrf_modem_lib.h>
#include <nrf_modem_at.h>
#include <modem/modem_info.h>
#include <helpers/nrfx_reset_reason.h>

#include <zephyr/logging/log.h>
/*---------------------------------------------------------------------------*/
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);
/*---------------------------------------------------------------------------*/
/* Struct that holds data from the modem information module. */
static struct modem_param_info modem_param;
/* Value that always holds the latest RSRP value. */
int32_t rsrp_value_latest;
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
	char reset_reason_str[128];

	/* Read RESETREAS register value and clear current reset reason(s). */
	reset_reason = nrfx_reset_reason_get();
	nrfx_reset_reason_clear(reset_reason);

	reset_reason_str_get(reset_reason_str, reset_reason);

	LOG_ERR("Reset reason: %s", reset_reason_str);
}
/*---------------------------------------------------------------------------*/
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

	LOG_DBG("Incoming RSRP status message, RSRP value is %d",rsrp_value_latest);
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
/*---------------------------------------------------------------------------*/
int main(void)
{
	int err;

	LOG_INF("Stratus Pro Device info sample started");

	/* Reset reason can only be read once, because the register needs to be cleared after
	 * reading.
	 */
	print_reset_reason();

	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Modem library initialization failed, error: %d\n", err);
		return 0;
	}

#if CONFIG_MODEM_INFO
	err = modem_data_init();
	if (err) {
		LOG_ERR("modem_data_init, error: %d\n", err);
	}
#endif /* CONFIG_MODEM_INFO */


	return 0;
}
