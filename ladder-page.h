/*
 * SPDX-FileCopyrightText: 2021-2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef LADDER_PAGE_H
#define LADDER_PAGE_H

#include "generic-strip.h"

typedef struct _LadderPage LadderPage;
typedef struct _LadderPageDescriptor LadderPageDescriptor;
typedef enum {LocationLeft, LocationRight} LadderPageRulerLocation;


struct _LadderPage{
    GenericStrip super;

    int idx; /**< Index of the page within the infinite strip. */
    LadderPageDescriptor *descriptor; /**< Weak reference to the descriptor that created this page. */
};

#define LADDER_PAGE(self) ((LadderPage*)(self))
#define LADDER_PAGE_DESCRIPTOR(self) ((LadderPageDescriptor*)(self))


LadderPage *ladder_page_new(LadderPageDescriptor *descriptor, int index);
LadderPage *ladder_page_init(LadderPage *self);
void *ladder_page_free(LadderPage *self);

static inline int ladder_page_get_index(LadderPage *self)
{
    return self->idx;
}

float ladder_page_resolve_value(LadderPage *self, float value);
#endif /* LADDER_PAGE_H */
