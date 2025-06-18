/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

#define SLEEP_TIME_MS 1000

#define INPUT_NODE DT_ALIAS(pininput)
static const struct gpio_dt_spec input_pin = GPIO_DT_SPEC_GET_OR(INPUT_NODE, gpios, {0});

int main(void)
{
    int ret;

    if (!device_is_ready(input_pin.port)) {
        LOG_ERR("GPIO device not ready");
        return 1;
    }

    ret = gpio_pin_configure_dt(&input_pin, GPIO_INPUT);
    if (ret != 0) {
        LOG_ERR("Failed to configure input pin (err %d)", ret);
        return 1;
    }

    while (1) {
        int val = gpio_pin_get_dt(&input_pin);
        if (val < 0) {
            LOG_ERR("Error reading inout pin value: %d", val);
        } else {
            LOG_INF("GPIO P0.18 state: %d\n", val);
        }

        k_msleep(SLEEP_TIME_MS);
    }

    return 0; // not reached
}

