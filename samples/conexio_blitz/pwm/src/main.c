/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
 
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Include the relevant headers for pwm */
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

LOG_MODULE_REGISTER(pwm_sample, LOG_LEVEL_INF);

/* Define the desired PWM period and pulse */
#define PWM_PERIOD_NS   200000000
#define PWM_DUTY_CYCLE  1400000


/* Get the node identifier for [] through its alias */
#define PWM_LED0     DT_ALIAS(pwm_led0)

/* Initialize and populate struct pwm_dt_spec */
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(PWM_LED0);

int main(void)
{
    LOG_INF("PWM sample running on %s\n", CONFIG_BOARD);

    int err;

    /* Check if the device is ready */
    if (!pwm_is_ready_dt(&pwm_led0)) {
        LOG_ERR("Error: PWM device %s is not ready", pwm_led0.dev->name);
        return 0;
	}
    
    /* Control the LED with the control signal generated from the PWM */
    err = pwm_set_dt(&pwm_led0, PWM_PERIOD_NS, PWM_DUTY_CYCLE);
    if (err) {
	LOG_ERR("Error in pwm_set_dt(), err: %d", err);
	return 0;
    }
    
    return 0;
}
