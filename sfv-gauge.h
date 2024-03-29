/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef SFV_GAUGE_H
#define SFV_GAUGE_H

#include "base-gauge.h"

typedef struct{
    BaseGauge super;

    float value;
}SfvGauge;

#define SFV_GAUGE(self) ((SfvGauge *)(self))


bool sfv_gauge_set_value(SfvGauge *self, float value, bool animated);
float sfv_gauge_get_value(SfvGauge *self);
#endif /* SFV_GAUGE_H */
