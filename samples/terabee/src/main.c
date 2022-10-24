/*---------------------------------------------------------------------------*/
/*
 * Copyright (c) 2022 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */   
/*---------------------------------------------------------------------------*/
#include <zephyr.h>
#include <stdio.h>
#include "terabee.h"

#define SAMPLING_INTERVAL   (K_SECONDS(5))

/* Variable to temp store measurement data */
uint16_t distance_mm = 0;

void main(void) 
{
	printf("Conexio Stratus Terabee sample started\n"); 

	/* Initialize I2C and the sensor */
    terabee_init();

    while (1)
    {
        /* Trigger distance measurement */
        distance_mm = get_distance_measurement();
    
        /* Finally, let's print measurement value to the serial console */
        printf("Distance: %d mm\n", distance_mm);

        k_sleep(SAMPLING_INTERVAL);
    }
}
