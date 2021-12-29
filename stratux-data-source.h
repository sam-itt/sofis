/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef STRATUX_DATA_SOURCE_H
#define STRATUX_DATA_SOURCE_H
#include "data-source.h"
#include "http-buffer.h"

typedef struct{
    DataSource super;

    HttpBuffer *buf;
}StratuxDataSource;


StratuxDataSource *stratux_data_source_new();
StratuxDataSource *stratux_data_source_init(StratuxDataSource *self);
#endif /* STRATUX_DATA_SOURCE_H */
