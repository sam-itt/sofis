/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef HTTP_DOWNLOAD_H
#define HTTP_DOWNLOAD_H
#include <stdbool.h>

bool http_download_file(char *url, char *output);

#endif /* HTTP_DOWNLOAD_H */
