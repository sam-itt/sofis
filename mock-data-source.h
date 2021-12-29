/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MOCK_DATA_SOURCE_H
#define MOCK_DATA_SOURCE_H
#include "data-source.h"

typedef struct{
    DataSource super;
}MockDataSource;

MockDataSource *mock_data_source_new(void);
MockDataSource *mock_data_source_init(MockDataSource *self);

#endif /* MOCK_DATA_SOURCE_H */
