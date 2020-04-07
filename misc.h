#ifndef MISC_H
#define MISC_H
#include <stdio.h>
#include <stdint.h>

typedef uint_fast8_t uintf8_t;

int number_split(float n, float *parts, size_t p_size);
int number_digits(float n);
#endif /* MISC_H */
