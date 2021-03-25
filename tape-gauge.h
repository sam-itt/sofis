/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef TAPE_GAUGE_H
#define TAPE_GAUGE_H

#include "base-gauge.h"
#include "ladder-gauge.h"
#include "odo-gauge.h"

typedef enum{
    AlignLeft,
    AlignRight
}Alignment;

typedef struct{
    BaseGauge super;

    LadderGauge *ladder;
    OdoGauge *odo;
}TapeGauge;

TapeGauge *tape_gauge_new(LadderPageDescriptor *descriptor,
                          Alignment align, int xoffset,
                          int nbarrels, ...);
TapeGauge *tape_gauge_vainit(TapeGauge *self, LadderPageDescriptor *descriptor,
                             Alignment align, int xoffset,
                             int nbarrels, va_list ap);

bool tape_gauge_set_value(TapeGauge *self, float value, bool animated);
float tape_gauge_get_value(TapeGauge *self);
#endif /* TAPE_GAUGE_H */
