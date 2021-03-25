/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef LSM303_H
#define LSM303_H

typedef struct{
    int fd;

}Lsm303;

Lsm303 *lsm303_init(Lsm303 *self, const char *bus);
Lsm303 *lsm303_dispose(Lsm303 *self);

void lsm303_start_magnetometer(Lsm303 *self);
bool lsm303_get_heading(Lsm303 *self, double roll, double pitch, double *heading);
#endif
