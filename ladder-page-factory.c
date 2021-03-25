/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "ladder-page-factory.h"

LadderPage *ladder_page_factory_create(int index, LadderPageDescriptor *descriptor)
{
    LadderPage *rv;
    float start;

    start = index * descriptor->page_size; /*'nominal' start, will be offsted by the init func */

    rv = ladder_page_new(start, descriptor);

    return descriptor->init_page(rv);
}
