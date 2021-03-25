/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef FB_PAGE_DESCRIPTOR_H
#define FB_PAGE_DESCRIPTOR_H

#include "ladder-page.h"

typedef struct{
    LadderPageDescriptor super;

    char *filename;
}FBPageDescriptor;


FBPageDescriptor *fb_page_descriptor_init(FBPageDescriptor *self, const char *filename, ScrollType direction, float page_size, float vstep, float vsubstep);
LadderPage *fb_ladder_page_init(LadderPage *self);

#endif /* FB_PAGE_DESCRIPTOR_H */
