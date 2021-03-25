/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef ALT_LADDER_PAGE_DESCRIPTOR_H
#define ALT_LADDER_PAGE_DESCRIPTOR_H

#include "ladder-page.h"
#include "fb-page-descriptor.h"

typedef struct{
    FBPageDescriptor super;
}AltLadderPageDescriptor;


AltLadderPageDescriptor *alt_ladder_page_descriptor_new(void);
LadderPage *alt_ladder_page_init(LadderPage *self);
#endif /* ALT_LADDER_PAGE_DESCRIPTOR_H */
