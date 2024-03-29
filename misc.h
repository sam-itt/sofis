/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef MISC_H
#define MISC_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "SDL_rect.h"

/* Note: Align NONE/LEFT and NONE/TOP are
 * equivalent.
 * */
#define HALIGN_NONE   0x00 /*xxxxvv00*/
#define HALIGN_LEFT   0x01 /*xxxxvv01*/
#define HALIGN_RIGHT  0x02 /*xxxxvv10*/
#define HALIGN_CENTER 0x03 /*xxxxvv11*/
#define HALIGN_MASK   0x03 /*00000011*/

#define VALIGN_NONE   0x00  /*xxxx00hh*/
#define VALIGN_TOP    0x04  /*xxxx01hh*/
#define VALIGN_BOTTOM 0x08  /*xxxx10hh*/
#define VALIGN_MIDDLE 0x0c  /*xxxx11hh*/
#define VALIGN_MASK   0x0c  /*00001100*/

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))


typedef int_fast8_t intf8_t;
typedef uint_fast8_t uintf8_t;
typedef uint_fast16_t uintf16_t;

int number_split(float n, float *parts, size_t p_size);
int number_digits(float n);

bool interval_intersect(float as, float ae, float bs, float be, float *is, float *ie);

void filter_dedup(char *base, size_t len);

char *nibble_spaces(const char *str, size_t n);
size_t split_str(const char *str, int (*splitter)(int c), char **parts, size_t nparts);

void mkdir_p(const char *dir, mode_t mode);
bool create_path(const char *filename);

void SDLExt_RectCenter(SDL_Rect *self, SDL_Rect *reference);
void SDLExt_RectAlign(SDL_Rect *self, SDL_Rect *reference, uint8_t alignment);
void SDLExt_RectDump(SDL_Rect *self);
#define SDLExt_RectLastX(rect) ((rect)->x + (rect)->w - 1)
#define SDLExt_RectLastY(rect) ((rect)->y + (rect)->h - 1)
#define SDLExt_RectMidY(rect) ((rect)->y + roundf(((rect)->h-1)/2.0f))
#define SDLExt_RectMidX(rect) ((rect)->x + roundf(((rect)->w-1)/2.0f))

static inline int clamp(int x, int low, int high)
{
    return (x > high) ? high : ((x < low) ? low : x);
}

static inline float clampf(float x, float low, float high)
{
    return (x > high) ? high : ((x < low) ? low : x);
}
#endif /* MISC_H */
