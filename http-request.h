/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <stdbool.h>
#include "http-buffer.h"

bool http_request(const char *url, HttpBuffer **buffer);
#endif /* HTTP_REQUEST_H */
