/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <sys/stat.h>
#include <libgen.h>

#include "misc.h"



/**
 * Separate a base-10 number into its components: units,
 * tens, hundreds, etc. The values will be laid out as
 * n = parts[0] * 10^0 + parts[1]*10^2 ... + parts[x]*10^x
 *       (units)         (tens)        ...    (rank x)
 * i.e, the index in the array is the rank of the value
 * stored at that index.
 *
 * @param n the value to split out
 * @parts parts a pointer to a float array with enough
 * space to hold all splitted out digits.
 * @param p_size number of parts that @param parts can store
 * @return the number of base-10 ranks that have been found
 *
 */
int number_split(float n, float *parts, size_t p_size)
{
    int i = 1;
    float val;
    size_t rank = 0;
    int divider;

    memset(parts, 0, sizeof(float)*p_size);
    while(n > 0 && rank < p_size) {
        val = fmod(n, 10) * i;
        divider = pow(10, rank);
        parts[rank] = val /= divider;
//        printf("val: %f\n", val);
        n = floor(n / 10);
        i *= 10;
        rank++;
    }
    return rank;
}

/**
 * Count the number of digits in n
 *
 */
int number_digits(float n)
{
    int i = 1;
    size_t rank = 0;

    while(n > 0 ) {
        n = floor(n / 10);
        i *= 10;
        rank++;
    }
    return rank;
}

bool interval_intersect(float as, float ae, float bs, float be, float *is, float *ie)
{
    if(bs > ae || as > be)
        return false;

    *is = (as > bs) ? as : bs;
    *ie = (ae < be) ? ae : be;

    return true;
}

/**
 * @brief Remove duplicates characters from base. base must be sorted
 * so that duplicates follow each other (i.e. use qsort() beforehand).
 *
 * @param base The string to filter
 * @param len The string len, -1 to have the function compute it.
 */
void filter_dedup(char *base, size_t len)
{
    len = len > -1 ? len : strlen(base);

    for(int i = 1; i < len; i++){
        if(base[i] == base[i-1]){
            int next; /*Next different char idx*/
            for(next = i; next < len && base[next] == base[i]; next++);
            /* Index doesn't go OOB, last iteration will
             * access (and move back) the final '\0' */
            for(int j = next; j < len+1; j++){
                base[i+(j-next)] = base[j];
            }
        }
    }
}

/**
 * @brief Returns the address of the first non-white space char in @parm str
 *
 * @param str The string to work on
 * @param n go to at most n chars. 0 will be treated as strlen(str)
 * @return Pointer to the first non-white space char (within str) or NULL if
 * the whole string was whitespace
 */
char *nibble_spaces(const char *str, size_t n)
{
    char *iter;
    const char *max;

    max = (n > 0) ? str+n : str+strlen(str);
    for(iter = (char*)str; isspace(*iter) && iter < max; iter++);

    return (iter != max-1) ? iter : NULL;
}

/**
 * @brief Break a delimited (comma, space, etc.) string into a sequence of substrings.
 *
 * This function will fill @param parts with pointers to the begining of each of the
 * detected parts. For example, a string like "one two three" will be splitted in 3
 * parts by a call to split_str using isspace(3) as splitter. The first pointer will
 * point to the begining of the string, the second to the 't' of two and the third
 * to the 't' of three.
 *
 * @param str The string to split
 * @param splitter the function to use as the splitter. The libc provide many, refer
 * to man isalpha(3) for a list.
 * @param parts The array to be filled-in. If NULL, it won't be filled and the function
 * will return the number of parts detected.
 * @param nparts The number of pointers that @param parts can hold. Ignored if @param
 * parts is NULL.
 * @return The number of splitted/splittable elements.
 */
size_t split_str(const char *str, int (*splitter)(int c), char **parts, size_t nparts)
{
    char *iter;
    const char *max;
    size_t rv;

    rv = 0;
    iter = (char*)str;
    max = str+strlen(str);
    do{
        while(splitter(*iter) && iter < max) iter++;
        if(parts && rv < nparts)
            parts[rv] = iter;
        rv++;
        while(!splitter(*iter) && iter < max) iter++;
    }while(iter < max);
    return rv-1; //Last increment is 'optimistic'
}


/**
 * @brief Creates directory @param dir, creating parents as needed.
 *
 * Same behavior as the shell command mkdir -p
 *
 *
 * @param dir The directory (path/hierarchy) to create
 * @param mode The creation mode, bitfield of permissions.
 * Refer to stat(2).
 *
 * credits: http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
 */
void mkdir_p(const char *dir, mode_t mode)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++){
        if(*p == '/') {
            *p = 0;
            mkdir(tmp, mode);
            *p = '/';
        }
    }
    mkdir(tmp, mode);
}

/**
 * @brief Create all directories leading to @param filename
 *
 * @param filename Path to a file.
 */
bool create_path(const char *filename)
{
    char *tmp;
    char *dname;
    size_t len;

    /*dirname can modify its argument, so we need to make a copy*/
    len = strlen(filename);
    if(len < 1024)
        tmp = strdupa(filename);
    else
        tmp = strdup(filename);
    dname = dirname(tmp);

    if(access(dname, F_OK) != 0)
        mkdir_p(dname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(len >= 1024) free(tmp);

    return access(dname, F_OK) == 0;
}

/**
 * Centers self on/in the reference rectangle. Self width
 * and height *must* be set.
 *
 * @param self the SDL_Rect that is worked on. Must have w and h set.
 * @param reference the rectangle that will have self in its center
 */
void SDLExt_RectCenter(SDL_Rect *self, SDL_Rect *reference)
{
    self->y = reference->y + round(reference->h/2.0) - round(self->h/2.0) -1;
    self->x = reference->x + round(reference->w/2.0) - round(self->w/2.0) -1;
}

/**
 * Aligns self with/in the reference rectangle. Self width
 * and height *must* be set. Alignemnt is controled by @param alignement
 *
 * @param self the SDL_Rect that is worked on. Must have w and h set.
 * @param reference the rectangle that will have self in its center
 * @param alignment bitwise combination of at most 2 VALIGN and HALIGN
 * constants. Example: HALIGN_CENTER | VALIGN_MIDDLE. Passing 0 means TOP/LEFT
 * alignment.
 */
void SDLExt_RectAlign(SDL_Rect *self, SDL_Rect *reference, uint8_t alignment)
{
    uint8_t valign, halign;

    halign = alignment & HALIGN_MASK;
    valign = (alignment & VALIGN_MASK);

    /*Default alignment: left/bottom*/
    self->x = reference->x;
    self->y = reference->y;

    if(halign == HALIGN_RIGHT){
        self->x += reference->w - self->w;
    }else if(halign == HALIGN_CENTER){
        self->x += round(reference->w/2.0) - round(self->w/2.0);
    }

    if(valign == VALIGN_BOTTOM){
        self->y += reference->h - self->h;
    }else if(valign == VALIGN_MIDDLE){
        self->y +=  round(reference->h/2.0) - round(self->h/2.0);
    }
}


void SDLExt_RectDump(SDL_Rect *self)
{
    printf("SDL_Rect(%p): x:%d, y:%d, w:%d, h:%d\n",self,self->x,self->y,self->w,self->h);
}
