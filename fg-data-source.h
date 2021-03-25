/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef FG_DATA_SOURCE_H
#define FG_DATA_SOURCE_H

#include "data-source.h"
#include "flightgear-connector.h"

typedef struct{
    DataSource super;

    FlightgearConnector *fglink;
    int port;
}FGDataSource;

FGDataSource *fg_data_source_new(int port);
FGDataSource *fg_data_source_init(FGDataSource *self, int port);

void fg_data_source_banner(FGDataSource *self);
#endif /* FG_DATA_SOURCE_H */
