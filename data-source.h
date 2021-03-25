/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H
#include <stdint.h>
#include <stdbool.h>

#include "basic-hud.h"
#include "map-gauge.h"
#include "side-panel.h"

#define DATA_SOURCE(self) ((DataSource*)self)

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

typedef struct _DataSource{
    DataSourceOps *ops;

    double latitude;
    double longitude;
    float altitude;

    float roll;
    float pitch;
    float heading;

    float airspeed; //kts
    float vertical_speed; //vertical speed //feets per second
    float slip_rad;

    float rpm;
    float fuel_flow;
    float oil_temp;
    float oil_press;
    float cht;
    float fuel_px;
    float fuel_qty;
}DataSource;

static inline bool data_source_frame(DataSource *self, uint32_t dt)
{
    return self->ops->frame(self, dt);
}

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

static inline DataSource *data_source_free(DataSource *self)
{
    free(data_source_dispose(self));
    return NULL;
}

#endif /* DATA_SOURCE_H */
