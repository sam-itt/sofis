/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H
#include <time.h>
#include <pthread.h>

#include <gps.h>

typedef struct{
    struct gps_data_t gpsdata;
    time_t timeout;

    pthread_t tid;
    pthread_mutex_t mtx;

    double latitude;
    double longitude;
    double altitude;
}GpsSensor;

GpsSensor *gps_sensor_new(const char *server, const char *port);
GpsSensor *gps_sensor_init(GpsSensor *self, const char *server, const char *port);
GpsSensor *gps_sensor_dispose(GpsSensor *self);

int gps_sensor_start(GpsSensor *self);
bool gps_sensor_get_fix(GpsSensor *self, double *latitude, double *longitude, double *altitude);
#endif /* GPS_SENSOR_H */
