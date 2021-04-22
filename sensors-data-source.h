/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef SENSORS_DATA_SOURCE_H
#define SENSORS_DATA_SOURCE_H

#include "data-source.h"
#include "sensors/bno080/bno080.h"
#include "sensors/gps-sensor.h"

typedef struct{
    DataSource super;

    Bno080 imu;
    GpsSensor gps;
}SensorsDataSource;

SensorsDataSource *sensors_data_source_new(void);
SensorsDataSource *sensors_data_source_init(SensorsDataSource *self);

#endif /* SENSORS_DATA_SOURCE_H */
