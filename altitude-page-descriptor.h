/*
 * SPDX-FileCopyrightText: 2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ALTITUDE_PAGE_DESCRIPTOR_H
#define ALTITUDE_PAGE_DESCRIPTOR_H
#include <stdint.h>

#include "ladder-page-descriptor.h"

typedef struct{
    LadderPageDescriptor super;
}AltitudePageDescriptor;

AltitudePageDescriptor *altitude_page_descriptor_new(int page_w, int page_h_min);
LadderPage *altitude_ladder_page_init(LadderPage *self);


#endif /* ALTITUDE_PAGE_DESCRIPTOR_H */
