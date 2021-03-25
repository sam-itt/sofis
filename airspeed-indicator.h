/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef AIRSPEED_INDICATOR_H
#define AIRSPEED_INDICATOR_H

#include "base-gauge.h"
#include "tape-gauge.h"
#include "text-gauge.h"
#include "airspeed-page-descriptor.h"

#define RHO_0 1.225 /* kg/m3, Sea level ISA */

typedef struct{
    BaseGauge super;

    TapeGauge *tape;
    TextGauge *txt;

    int tas;
}AirspeedIndicator;


AirspeedIndicator *airspeed_indicator_new(speed_t v_so, speed_t v_s1,speed_t v_fe,speed_t v_no,speed_t v_ne);
AirspeedIndicator *airspeed_indicator_init(AirspeedIndicator *self, speed_t v_so, speed_t v_s1,speed_t v_fe,speed_t v_no,speed_t v_ne);

bool airspeed_indicator_set_value(AirspeedIndicator *self, float value);
#endif /* AIRSPEED_INDICATOR_H */
