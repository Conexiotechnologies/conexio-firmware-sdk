/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include "nrf_fuel_gauge.h"
#include "fuel_gauge.h"

#if defined(CONFIG_NRF_FUEL_GAUGE)

static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(pmic_charger));

/* nPM1300 CHARGER.BCHGCHARGESTATUS.CONSTANTCURRENT register bitmask */
#define NPM1300_CHG_STATUS_CC_MASK BIT_MASK(3)
FuelGaugeData pmic_data;

static bool initialized;
static float max_charge_current;
static float term_charge_current;
static int64_t ref_time;

static const struct battery_model battery_model = {
#include "battery_model.inc"
};

static int read_sensors(const struct device *charger,
	float *voltage, float *current, float *temp, int32_t *chg_status)
{
	struct sensor_value value;
	int ret;

	ret = sensor_sample_fetch(charger);
	if (ret < 0) {
		return ret;
	}

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &value);
	*voltage = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &value);
	*temp = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &value);
	*current = (float)value.val1 + ((float)value.val2 / 1000000);

	sensor_channel_get(charger, SENSOR_CHAN_NPM1300_CHARGER_STATUS, &value);
	*chg_status = value.val1;

	return 0;
}

static int fuel_gauge_init(const struct device *charger)
{
	struct sensor_value value;
	struct nrf_fuel_gauge_init_parameters parameters = {
		.model = &battery_model,
		.opt_params = NULL,
	};
	int32_t chg_status;
	int ret;

	printk("nRF Fuel Gauge version: %s\n", nrf_fuel_gauge_version);

	ret = read_sensors(charger, &parameters.v0, &parameters.i0, &parameters.t0, &chg_status);
	if (ret < 0) {
		return ret;
	}

	/* Store charge nominal and termination current, needed for ttf calculation */
	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &value);
	max_charge_current = (float)value.val1 + ((float)value.val2 / 1000000);
	term_charge_current = max_charge_current / 10.f;

	nrf_fuel_gauge_init(&parameters, NULL);

	ref_time = k_uptime_get();

	return 0;
}

void update_pmic_data(float voltage, float current, float soc, float tte, float ttf) {
    pmic_data.voltage = voltage;
    pmic_data.current = current;
    pmic_data.soc = soc;
    pmic_data.tte = tte;
    pmic_data.ttf = ttf;
}

int fuel_gauge_update(const struct device *charger, bool vbus_connected)
{
	float voltage;
	float current;
	float temp;
	float soc;
	float tte;
	float ttf;
	float delta;
	int32_t chg_status;
	bool cc_charging;
	int ret;

	ret = read_sensors(charger, &voltage, &current, &temp, &chg_status);
	if (ret < 0) {
		printk("Error: Could not read from charger device\n");
		return ret;
	}

	cc_charging = (chg_status & NPM1300_CHG_STATUS_CC_MASK) != 0;

	delta = (float) k_uptime_delta(&ref_time) / 1000.f;

	soc = nrf_fuel_gauge_process(voltage, current, temp, delta, vbus_connected, NULL);
	tte = nrf_fuel_gauge_tte_get();
	ttf = nrf_fuel_gauge_ttf_get(cc_charging, -term_charge_current);

	/* Stratus Pro does not have the NTC sensor connected, so dont display */
	printk("V: %.3f, I: %.3f, ", (double)voltage, (double)current);
	printk("SoC: %.2f, TTE: %.0f, TTF: %.0f\n", (double)soc, (double)tte, (double)ttf);

	update_pmic_data(voltage, current, soc, tte, ttf);

	return 0;
}

/**@brief Initialize fuel gauge and start. */
int fuel_gauge_init_and_start(void)
{
	printk("Init and start PMIC device\n");

	if (!device_is_ready(charger)) {
		printk("Charger device not ready.\n");
		initialized = false;
	}

	if (fuel_gauge_init(charger) < 0) {
		printk("Could not initialise fuel gauge.\n");
		initialized = false;
	}

	initialized = true;
	printk("PMIC device init successful\n");

	return initialized;
}

#endif /* CONFIG_NRF_FUEL_GAUGE */
