/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef JY61_H
#define JY61_H
#include <stdbool.h>
#include <pthread.h>

typedef struct{
    int fd;

    char r_buf[1024];

    pthread_t reader_tid;
    pthread_mutex_t angle_mutex;
}JY61;

JY61 *jy61_init(JY61 *self, const char *device);
JY61 *jy61_dispose(JY61 *self);
int jy61_start(JY61 *self);
void jy61_get_attitude(JY61 *self, double *roll, double *pitch, double *yaw);

#endif /* JY61_H */
