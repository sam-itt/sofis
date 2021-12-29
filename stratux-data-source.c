/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "stratux-data-source.h"
#include "http-request.h"

#include "misc.h"
#include <math.h>

//#define API_ENDPOINT "http://127.0.0.1/getSituation"
#define API_ENDPOINT "http://192.168.10.1/getSituation"

char *json_get_value(const char *json, const char *key, size_t *keylen);
double json_get_double_value(const char *json, const char *key, const char *nan_value);


static bool stratux_data_source_frame(StratuxDataSource *self, uint32_t dt);
static StratuxDataSource *stratux_data_source_dispose(StratuxDataSource *self);
static DataSourceOps stratux_data_source_ops = {
    .frame = (DataSourceFrameFunc)stratux_data_source_frame,
    .dispose = (DataSourceDisposeFunc)stratux_data_source_dispose
};


StratuxDataSource *stratux_data_source_new(int port)
{
    StratuxDataSource *self;

    self = calloc(1, sizeof(StratuxDataSource));
    if(self){
        if(!stratux_data_source_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}

StratuxDataSource *stratux_data_source_init(StratuxDataSource *self)
{
    if(!data_source_init(DATA_SOURCE(self), &stratux_data_source_ops))
        return NULL;
    self->buf = http_buffer_new(0);
    if(!self->buf)
        return NULL;

    return self;
}

static StratuxDataSource *stratux_data_source_dispose(StratuxDataSource *self)
{
    if(self->buf){
        if(self->buf->buffer)
            free(self->buf->buffer);
        free(self->buf);
    }
    return self;
}


static bool stratux_data_source_frame(StratuxDataSource *self, uint32_t dt)
{
    bool rv;
    double lat, lon, alt;
    double roll, pitch,heading, mheading;
    double vertical_speed_gps;
    double vertical_speed_baro;

    rv = http_request(API_ENDPOINT, &self->buf);
    if(!rv) return false;

    lat = json_get_double_value(self->buf->buffer, "GPSLatitude", NULL);
    lon = json_get_double_value(self->buf->buffer, "GPSLongitude", NULL);
    alt = json_get_double_value(self->buf->buffer, "GPSHeightAboveEllipsoid", NULL);

    roll = json_get_double_value(self->buf->buffer, "AHRSRoll", "3276.7");
    pitch = json_get_double_value(self->buf->buffer, "AHRSPitch", "3276.7");
    heading = json_get_double_value(self->buf->buffer, "AHRSGyroHeading", "3276.7");
    mheading = json_get_double_value(self->buf->buffer, "AHRSMagHeading", "3276.7");

    vertical_speed_gps = json_get_double_value(self->buf->buffer, "GPSVerticalSpeed", NULL);
    vertical_speed_baro = json_get_double_value(self->buf->buffer, "BaroVerticalSpeed", NULL);

    if(!isnan(heading))
        heading = fmod(heading, 360.0);
    if(!isnan(mheading))
        mheading = fmod(mheading, 360.0);
    data_source_set_location(
        DATA_SOURCE(self), &(LocationData){
            .super.latitude = lat,
            .super.longitude = lon,
            .altitude = alt
        }
    );

    data_source_set_dynamics(
        DATA_SOURCE(self), &(DynamicsData){
            .airspeed = DATA_SOURCE(self)->dynamics.airspeed,
            .vertical_speed = vertical_speed_gps,
            .slip_rad = DATA_SOURCE(self)->dynamics.slip_rad
        }
    );


    float new_heading;
    if(!isnan(heading))
        new_heading = fmod(heading,360.0);
    else if(!isnan(mheading))
        new_heading = fmod(mheading,360.0);

    data_source_set_attitude(
        DATA_SOURCE(self), &(AttitudeData){
            .roll = roll,
            .pitch = pitch,
            .heading = new_heading
        }
    );

#if 0
    printf("lat: %f lon: %f, alt: %f\n"
        "roll: %f pitch: %f heading(gyro): %f heading(mag): %f\n",
        lat, lon, alt,
        roll, pitch, heading, mheading
    );
#endif
    self->buf->len = 0;

    DATA_SOURCE(self)->has_fix = true;
    return true;
}

/**
 * This is very primitive and won't work with complex (i.e.
 * nested arrays/objects) types
 *
 */
char *json_get_value(const char *json, const char *key, size_t *keylen)
{
    char *rv;
    char *kend;

    rv = strstr(json, key);
    if(!rv) return NULL;

    rv = strchr(rv, ':');
    if(!rv) return NULL;

    rv = nibble_spaces(rv+1, 0);
    if(!rv) return NULL;

    kend = strchr(rv, ',');
    if(!kend)
        kend = strchr(rv, '}');
    if(kend && keylen)
        *keylen = kend - rv;

    return rv;
}

double json_get_double_value(const char *json, const char *key, const char *nan_value)
{
    double rv;
    char *strval;
    size_t len;

    strval = json_get_value(json, key, &len);
    if(!strval) return NAN;

    if(nan_value && !strncmp(strval, nan_value, len)) return NAN;

    return strtod(strval, NULL);
}

