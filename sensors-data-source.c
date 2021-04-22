/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <gps.h>
#include <stdio.h>
#include <stdlib.h>

#include "sensors-data-source.h"
#include "sensors/gps-sensor.h"

#ifndef BNO080_DEV
#define BNO080_DEV "/dev/i2c-1"
#endif

static bool sensors_data_source_frame(SensorsDataSource *self, uint32_t dt);
static SensorsDataSource *sensors_data_source_dispose(SensorsDataSource *self);
static DataSourceOps sensors_data_source_ops = {
    .frame = (DataSourceFrameFunc)sensors_data_source_frame,
    .dispose = (DataSourceDisposeFunc)sensors_data_source_dispose
};

SensorsDataSource *sensors_data_source_new(void)
{
    SensorsDataSource *self;

    self = calloc(1, sizeof(SensorsDataSource));
    if(self){
        if(!sensors_data_source_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}

SensorsDataSource *sensors_data_source_init(SensorsDataSource *self)
{
    if(!data_source_init(DATA_SOURCE(self), &sensors_data_source_ops))
        return NULL;

    if(!bno080_init(&self->imu, 0x4b, BNO080_DEV)){
        printf("Couldn't initialize BNO0808 device, bailing out\n");
        exit(EXIT_FAILURE);
    }
    bno080_enable_feature(&self->imu, ROTATION_VECTOR);


    if(!gps_sensor_init(&self->gps, "localhost", DEFAULT_GPSD_PORT)){
        printf("Couldn't initialize GPS, bailing out\n");
        exit(EXIT_FAILURE);
    }

    gps_sensor_start(&self->gps);

    DATA_SOURCE(self)->latitude = 45.215470;
    DATA_SOURCE(self)->longitude = 5.844828;
    DATA_SOURCE(self)->altitude = 718.267245;

    DATA_SOURCE(self)->heading = 43.698940;

    return self;
}

static SensorsDataSource *sensors_data_source_dispose(SensorsDataSource *self)
{
    bno080_dispose(&self->imu);
    return self;
}

static bool sensors_data_source_frame(SensorsDataSource *self, uint32_t dt)
{
    double roll, pitch, yaw;
    double heading;
    double lat, lon, alt;
    bool rv;

    if(dt != 0 && dt < (1000/25)) //One update per 1/25 second
        return false;

    bno080_hpr(&self->imu, &heading, &pitch, &roll);

    DATA_SOURCE(self)->roll = roll;
    DATA_SOURCE(self)->pitch = pitch;
    DATA_SOURCE(self)->heading = heading;
    gps_sensor_get_fix(&self->gps, &lat, &lon, &alt);
    DATA_SOURCE(self)->latitude = lat;
    DATA_SOURCE(self)->longitude = lon;
    DATA_SOURCE(self)->altitude = alt*3.281; /*Comes in meters(gps), must be in feets*/

    return true;
}
