/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>

#include "gps-sensor.h"
#define GPSD_API_SWITCH 9

static void gps_sensor_set_fix(GpsSensor *self);
static void gps_sensor_worker(GpsSensor *self);

#if GPSD_API_MAJOR_VERSION >= GPSD_API_SWITCH
static inline bool timespec_equal(struct timespec *t1, struct timespec *t2)
{
    return t1->tv_sec == t2->tv_sec && t1->tv_nsec == t2->tv_nsec;
}
#else
static inline bool timespec_equal(double *t1, double *t2)
{
    return *t1 == *t2;
}
#endif

GpsSensor *gps_sensor_new(const char *server, const char *port)
{
    GpsSensor *self;

    self = calloc(1, sizeof(GpsSensor));
    if(self){
        if(!gps_sensor_init(self, server, port)){
            free(gps_sensor_dispose(self));
            return NULL;
        }
    }
    return self;
}

GpsSensor *gps_sensor_init(GpsSensor *self, const char *server, const char *port)
{
    int rv;

    rv = gps_open(server, port, &self->gpsdata);
    if(rv != 0){
        printf("Couldn't connect to gpsd error %d (%s)\n",
            errno,
            gps_errstr(errno)
        );
        return NULL;
    }
    gps_stream(&self->gpsdata, WATCH_ENABLE, NULL);

    self->timeout = 5;      /* seconds */
    self->latitude = NAN;

    pthread_mutex_init(&self->mtx, NULL);

    return self;
}

GpsSensor *gps_sensor_dispose(GpsSensor *self)
{
    pthread_cancel(self->tid);
    gps_close(&self->gpsdata);
    return self;
}

int gps_sensor_start(GpsSensor *self)
{
    return pthread_create(&self->tid, NULL, (void*)gps_sensor_worker, self);
}

bool gps_sensor_get_fix(GpsSensor *self, double *latitude, double *longitude, double *altitude)
{
    pthread_mutex_lock(&self->mtx);
    *latitude = self->latitude;
    *longitude = self->longitude;
    *altitude = self->altitude;
#if 0
    printf("lat: %f lon: %f alt: %f\n",
		 self->gpsdata.fix.latitude,
         self->gpsdata.fix.longitude,
         self->gpsdata.fix.altitude
    );
#endif
    pthread_mutex_unlock(&self->mtx);
    return true;
}

static void gps_sensor_set_fix(GpsSensor *self)
{
#if GPSD_API_MAJOR_VERSION >= GPSD_API_SWITCH
    struct timespec old_time;
#else
    double old_time;
#endif
    static double old_lat, old_lon;
    static bool first = true;

    if(timespec_equal(&(self->gpsdata.fix.time), &old_time) || self->gpsdata.fix.mode < MODE_2D)
        return;
    old_time = self->gpsdata.fix.time;

    if(self->gpsdata.fix.latitude == old_lat && self->gpsdata.fix.longitude == old_lon)
        return;
	old_lat = self->gpsdata.fix.latitude;
	old_lon = self->gpsdata.fix.longitude;

    pthread_mutex_lock(&self->mtx);
    self->latitude = self->gpsdata.fix.latitude;
    self->longitude = self->gpsdata.fix.longitude;
    self->altitude = self->gpsdata.fix.altitude;
#if 0
    printf("lat: %f lon: %f alt: %f\n",
		 self->gpsdata.fix.latitude,
         self->gpsdata.fix.longitude,
         self->gpsdata.fix.altitude
    );
#endif
    pthread_mutex_unlock(&self->mtx);
}

/*loosly modeled after gpsd's gps_mainloop*/
static void gps_sensor_worker(GpsSensor *self)
{
    int rv;

    for(;;){
        if(gps_waiting(&self->gpsdata, self->timeout * 1000000)){
#if GPSD_API_MAJOR_VERSION >= GPSD_API_SWITCH
            rv = gps_read(&self->gpsdata, NULL, 0);
#else
            rv = gps_read(&self->gpsdata);
#endif
            if(rv > 0){
                /*actually process data*/
                gps_sensor_set_fix(self);
            }
        }else{
            printf("Connection to gpsd lost, reconnecting in %ld seconds\n",
                self->timeout
            );
            sleep(self->timeout);
        }
    }
}
