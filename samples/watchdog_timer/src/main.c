/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2022 Conexio Technologies, Inc
 * 
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>
#include <logging/log.h>
#include "watchdog.h"
#include <stdlib.h>

LOG_MODULE_REGISTER(wdtimer_app, CONFIG_WDTIMER_LOG_LEVEL);

/* Stack definition for application workqueue */
K_THREAD_STACK_DEFINE(application_stack_area,
		      CONFIG_APPLICATION_WORKQUEUE_STACK_SIZE);
static struct k_work_q application_work_q;

void main(void)
{

	LOG_INF("Stratus watchdog timer sample");

	k_work_queue_start(&application_work_q, application_stack_area,
		       K_THREAD_STACK_SIZEOF(application_stack_area),
		       CONFIG_APPLICATION_WORKQUEUE_PRIORITY, NULL);

	if (IS_ENABLED(CONFIG_WATCHDOG)) {
		watchdog_init_and_start(&application_work_q);
	}

}
