#ifndef MISC_H
#define MISC_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint_fast8_t uintf8_t;
typedef uint_fast16_t uintf16_t;

int number_split(float n, float *parts, size_t p_size);
int number_digits(float n);

bool interval_intersect(float as, float ae, float bs, float be, float *is, float *ie);
#endif /* MISC_H */
