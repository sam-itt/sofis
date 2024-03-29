/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ALT_GROUP_H
#define ALT_GROUP_H

#include "base-gauge.h"
#include "alt-indicator.h"
#include "vertical-stair.h"

typedef struct{
    BaseGauge super;

    AltIndicator *altimeter;
    VerticalStair *vsi;
}AltGroup;

AltGroup *alt_group_new(void);
AltGroup *alt_group_init(AltGroup *self);

void alt_group_set_altitude(AltGroup *self, float value);
void alt_group_set_vertical_speed(AltGroup *self, float value);
void alt_group_set_values(AltGroup *self, float alt, float vs);

#endif /* ALT_GROUP_H */
