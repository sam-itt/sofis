/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef LADDER_PAGE_FACTORY_H
#define LADDER_PAGE_FACTORY_H

#include "ladder-page.h"

LadderPage *ladder_page_factory_create(int index, LadderPageDescriptor *descriptor);
#endif /* LADDER_PAGE_FACTORY_H */
