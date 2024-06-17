/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 */

#ifndef __FUEL_GAUGE_H__
#define __FUEL_GAUGE_H__

typedef struct {
    float voltage;
    float current;
    float soc;
    float tte;
    float ttf;
} FuelGaugeData;

extern FuelGaugeData pmic_data;

int fuel_gauge_init_and_start(void);

int fuel_gauge_update(void);

#endif /* __FUEL_GAUGE_H__ */
