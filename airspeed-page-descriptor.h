/*
 * SPDX-FileCopyrightText: 2021-2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef AIRSPEED_PAGE_DESCRIPTOR_H
#define AIRSPEED_PAGE_DESCRIPTOR_H

#include "ladder-page-descriptor.h"

/* uintf8_t: Good for 250 knots. Real units show 240 kts.
 * change type (i.e uint_fast_16_t) for higher speeds
 * */
typedef uint_fast8_t speed_t;

#define MAX_AIRPSEED_COLOR_ZONES 4

typedef struct{
    LadderPageDescriptor super;

    speed_t v_so; /*white arc begin*/
    speed_t v_s1; /*green arc begin*/
    speed_t v_fe; /*white arc end*/
    speed_t v_no; /*green arc end, yellow arc begin*/
    speed_t v_ne; /*green arc end, yellow arc end, red line*/

    ColorZone zones[MAX_AIRPSEED_COLOR_ZONES];
}AirspeedPageDescriptor;


AirspeedPageDescriptor *airspeed_page_descriptor_new(int page_w, int page_h_min, speed_t v_so,
                                                     speed_t v_s1, speed_t v_fe, speed_t v_no,
                                                     speed_t v_ne);


#endif /* AIRSPEED_PAGE_DESCRIPTOR_H */
