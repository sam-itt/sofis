/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef BASIC_HUD_H
#define BASIC_HUD_H

#include "base-gauge.h"
#include "alt-group.h"
#include "attitude-indicator.h"
#include "airspeed-indicator.h"
#include "compass-gauge.h"
#include "data-source.h"

typedef enum{
    ALTITUDE,
    VERTICAL_SPEED,
    AIRSPEED,
    PITCH,
    ROLL,
    SLIP,
    HEADING,

    HUD_VALUE_MAX
}HudValue;

enum{
    ALT_GROUP,
    SPEED,
    COMPASS,

    LOC_MAX
};

typedef struct{
    BaseGauge super;

    AltGroup *altgroup;
    AirspeedIndicator *airspeed;
    AttitudeIndicator *attitude;
    CompassGauge *compass;

    SDL_Rect locations[LOC_MAX];
}BasicHud;


BasicHud *basic_hud_new(void);
BasicHud *basic_hud_init(BasicHud *self);

float basic_hud_get(BasicHud *self, HudValue hv);
void basic_hud_set(BasicHud *self, int nvalues, ...);
void basic_hud_set_values(BasicHud *self, int nvalues, va_list ap);

void basic_hud_attitude_changed(BasicHud *self, AttitudeData *newv);
void basic_hud_dynamics_changed(BasicHud *self, DynamicsData *newv);
void basic_hud_location_changed(BasicHud *self, LocationData *newv);
#endif /* BASIC_HUD_H */
