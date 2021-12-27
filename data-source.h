/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "geo-location.h"

#define MAX_LOCATION_LISTENERS 3
#define MAX_ATTITUDE_LISTENERS 3
#define MAX_DYNAMICS_LISTENERS 1
#define MAX_ENGINE_DATA_LISTENERS 1
#define MAX_ROUTE_DATA_LISTENERS 2
#define TOTAL_MAX_LISTENERS \
          MAX_LOCATION_LISTENERS \
        + MAX_ATTITUDE_LISTENERS \
        + MAX_DYNAMICS_LISTENERS \
        + MAX_ENGINE_DATA_LISTENERS \
        + MAX_ROUTE_DATA_LISTENERS

typedef struct _DataSource DataSource;
typedef bool (*DataSourceFrameFunc)(DataSource *self, uint32_t dt);
typedef DataSource *(*DataSourceDisposeFunc)(DataSource *self);

typedef struct{
    /**
     * @param dt elapsed milliseconds since previous frame
     */
    DataSourceFrameFunc frame;
    DataSourceDisposeFunc dispose;
}DataSourceOps;

typedef void (*ValueListenerFunc)(void *self, const void *newvalue);
typedef struct{
    ValueListenerFunc callback;
    void *target;
}ValueListener;

typedef enum{
    LOCATION_DATA,
    ATTITUDE_DATA,
    DYNAMICS_DATA,
    ENGINE_DATA,
    ROUTE_DATA,
    N_VALUE_TYPES
}DataType;

typedef struct{
    float roll;
    float pitch;
    float heading;
}AttitudeData;

typedef struct{
    float airspeed; //kts
    float vertical_speed; //vertical speed //feets per second
    float slip_rad;
}DynamicsData;

typedef struct{
    float rpm;
    float fuel_flow;
    float fuel_px;
    float oil_temp;
    float oil_press;
    float cht;
    float fuel_qty;
}EngineData;

typedef struct{
    GeoLocation super;
    float altitude;
}LocationData;

typedef struct{
    GeoLocation to;
    GeoLocation from;
}RouteData;

typedef struct _DataSource{
    DataSourceOps *ops;

    LocationData location;
    AttitudeData attitude;
    DynamicsData dynamics;
    EngineData engine_data;
    RouteData route;

    /* We want to avoid dynamic allocation for these.
     * Thus the adding functio will emit a warning at runtime if the values are
     * exceeded to let the developper know that the value should be
     * incremented.
     * The number of listeners is known at compile-time and won't change at
     * runtime
     */
    ValueListener listeners[TOTAL_MAX_LISTENERS];
    size_t nlisteners[N_VALUE_TYPES];

    bool has_fix;
}DataSource;

#define DATA_SOURCE(self) ((DataSource*)self)

DataSource *data_source_get_instance(void);
void data_source_set(DataSource *source);

bool data_source_add_listener(DataSource *self, DataType type, ValueListener *listener);
size_t data_source_add_events_listener(DataSource *self, void *target,
                                           size_t nevents, ...);
void data_source_print_listener_stats(DataSource *self);

void data_source_set_location(DataSource *self, LocationData *location);
void data_source_set_attitude(DataSource *self, AttitudeData *attitude);
void data_source_set_dynamics(DataSource *self, DynamicsData *dynamics);
void data_source_set_engine_data(DataSource *self, EngineData *engine_data);
void data_source_set_route_data(DataSource *self, RouteData *route_data);

static inline DataSource *data_source_init(DataSource *self, DataSourceOps *ops)
{
    self->ops = ops;
    return self;
}

static inline DataSource *data_source_dispose(DataSource *self)
{
    if(self->ops->dispose)
        return self->ops->dispose(self);
    return self;
}


static inline bool data_source_frame(DataSource *self, uint32_t dt)
{
    return self->ops->frame(self, dt);
}

static inline DataSource *data_source_free(DataSource *self)
{
    free(data_source_dispose(self));
    return NULL;
}

static inline bool location_equals(LocationData *a, LocationData *b)
{
    return   (a->super.latitude == b->super.latitude)
          && (a->super.longitude == b->super.longitude)
          && (a->altitude == b->altitude);
}

static inline bool attitude_equals(AttitudeData *a, AttitudeData *b)
{
    return   (a->roll == b->roll)
          && (a->pitch == b->pitch)
          && (a->heading == b->heading);
}

static inline bool dynamics_equals(DynamicsData *a, DynamicsData *b)
{
    return   (a->airspeed == b->airspeed)
          && (a->vertical_speed == b->vertical_speed)
          && (a->slip_rad == b->slip_rad);
}

static inline bool engine_data_equals(EngineData *a, EngineData *b)
{
    return   (a->rpm == b->rpm)
          && (a->fuel_flow == b->fuel_flow)
          && (a->fuel_px == b->fuel_px)
          && (a->oil_temp == b->oil_temp)
          && (a->oil_press == b->oil_press)
          && (a->cht == b->cht)
          && (a->fuel_qty == b->fuel_qty);
}

static inline bool route_data_equals(RouteData *a, RouteData *b)
{
    return    (a->to.latitude == b->to.latitude)
           && (a->to.longitude == b->to.longitude)
           && (a->from.latitude == b->from.latitude)
           && (a->from.longitude == b->from.longitude);
}
#endif /* DATA_SOURCE_H */
