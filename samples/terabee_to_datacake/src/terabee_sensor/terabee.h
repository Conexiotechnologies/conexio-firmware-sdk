/*---------------------------------------------------------------------------*/
/* Driver for Terabee Evo long range ToF distance sensor
 *
 * Copyright (c) 2022 Conexio Technologies, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * \author Rajeev Piyare <rajeev@conexiotech.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef ZEPHYR_SENSOR_TERABEE_H_
#define ZEPHYR_SENSOR_TERABEE_H_
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/* Terabee Configs */
#define TERABEE_I2C_ADDRESS         0x31
#define SENSOR_DATA_BUF_SIZE        3
/*---------------------------------------------------------------------------*/
int terabee_init(void);
uint16_t get_distance_measurement(void);
/*---------------------------------------------------------------------------*/
#endif /* ZEPHYR_SENSOR_TERABEE_H_ */
/*---------------------------------------------------------------------------*/
