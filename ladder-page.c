/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "generic-layer.h"
#include "ladder-page.h"
#include "sdl-colors.h"
#include "SDL_pcf.h"


LadderPageDescriptor *ladder_page_descriptor_init(LadderPageDescriptor *self, int width, int height,
                                                  ScrollType direction, float page_size, float vstep,
                                                  float vsubstep, LPInitFunc func)
{
    self->width = width;
    self->height = height;
    self->direction = direction;
    self->page_size = page_size;
    self->vstep = vstep;
    self->vsubstep = vsubstep;
    self->offset = NAN;
    self->init_page = func;

    return self;
}

/*TODO: Check if ppv could be intergrated into the descriptor*/
void ladder_page_descriptor_compute_offset(LadderPageDescriptor *self, float ppv)
{
    float half_split;
    int leading, trailing;
    float pixel_increment;

    pixel_increment = ppv * self->vstep; /*How many pixels between two ruler marks*/

    half_split = (pixel_increment-1)/2.0;

    if(self->direction == TOP_DOWN){ /*0 is at the top of the image, increments downwards*/
        leading = ceil(half_split);
        self->offset = -1.0 * leading*ppv;
    }else{ /*0 is at the bottom of the image, increments upwards*/
        trailing = floor(half_split);
        self->offset = -1.0 * trailing/ppv;
    }
}

LadderPage *ladder_page_descriptor_create_page(LadderPageDescriptor *self, int index)
{
    LadderPage *rv;
    float start;

    start = index * self->page_size; /*'nominal' start, will be offsted by the init func */

    rv = ladder_page_new(start, self);

    return self->init_page(rv);
}

LadderPage *ladder_page_new(float start, LadderPageDescriptor *descriptor)
{
    LadderPage *self;

    self = calloc(1, sizeof(LadderPage));
    if(self){
        VERTICAL_STRIP(self)->start = start;
        VERTICAL_STRIP(self)->end = NAN;
        self->descriptor = descriptor;
    }
    return self;
}


LadderPage *ladder_page_init(LadderPage *self)
{
    VerticalStrip *strip;
    GenericLayer *layer;
    LadderPageDescriptor *descriptor;

    strip = VERTICAL_STRIP(self);
    layer = GENERIC_LAYER(self);
    descriptor = LADDER_PAGE(self)->descriptor;

    bool rv;


    rv = generic_layer_init(GENERIC_LAYER(self), descriptor->width, descriptor->height);
    if(!rv){
        return NULL;
    }

    strip->ppv = generic_layer_h(layer)/(descriptor->page_size*1.0);
//    printf("LadderPageDescriptor ppv is %f\n",strip->ppv);
//    printf("Page marking range is [%f, %f]\n", strip->start, strip->end);

    /* We are just going to offset the interval, size remains the same
     * so ppv wont change and can be computed before or afterwards*/
    if(isnan(descriptor->offset)){
        ladder_page_descriptor_compute_offset(descriptor, strip->ppv);
    }

    strip->start += descriptor->offset;
    strip->end = strip->start + descriptor->page_size-1;

//    int page_index = ladder_page_get_index(LADDER_PAGE(self));
//    printf("Page %d real range is [%f, %f]\n",page_index, strip->start, strip->end);

    return self;
}

void ladder_page_free(LadderPage *self)
{
    vertical_strip_dispose(VERTICAL_STRIP(self));
    free(self);
}

int ladder_page_get_index(LadderPage *self)
{
    return ceil(VERTICAL_STRIP(self)->start/(self->descriptor->page_size));
}

float ladder_page_resolve_value(LadderPage *self, float value)
{
    VerticalStrip *strip;
    bool reverse;

    strip = VERTICAL_STRIP(self);
    reverse = (self->descriptor->direction == BOTTUM_UP);

    if(fmod(value, self->descriptor->vstep) == 0){ /*Value is a big graduation*/
        float y;
        value = fmod(value, fabs(round(strip->end - strip->start)) + 1);
        int ngrads = value/self->descriptor->vstep;
        if(!reverse)
            y = self->descriptor->fei + ngrads * strip->ppv * self->descriptor->vstep;
        else
            y = self->descriptor->fei - ngrads * strip->ppv * self->descriptor->vstep;
        return y;
    }else if(self->descriptor->vsubstep != 0 && fmod(value, self->descriptor->vsubstep) == 0){ /*Value is a small graduation*/
        float y;
        value = fmod(value, fabs(round(strip->end - strip->start)) + 1);
        int ngrads = value/self->descriptor->vsubstep;
        if(!reverse)
            y = self->descriptor->fei + ngrads * strip->ppv * self->descriptor->vsubstep;
        else
            y = self->descriptor->fei - ngrads * strip->ppv * self->descriptor->vsubstep;
        return y;
    }else{
        return vertical_strip_resolve_value(strip, value, reverse);
    }
}


void ladder_page_draw_ruler(LadderPage *self, uint8_t base_unit_px, LadderPageRulerLocation location,
                            uint8_t small_step_width, uint8_t big_step_width)
{
    SDL_Surface *surface;
    int x;
    Uint32 white;
    Uint32 *pixels;
    int firstx, lastx;
    int ticks_drawn;

    int n_steps;
    int step_px;

    int preload_px;
    int preload_ticks;

    int base_unit = self->descriptor->vsubstep;
    int marking_step = self->descriptor->vstep;


//    printf("Drawing ladder page ruler with base_unit: %d, base_unit_px: %d, marking_step: %d marking_step %% base_unit: %d\n", base_unit, base_unit_px, marking_step, marking_step % base_unit);
    assert(marking_step % base_unit == 0);

    /* This is not how many *ticks* there are, but how much base units *increments/intervals* you must walk
     * starting at a big tick to go to the next.
     * */
    n_steps = marking_step/base_unit;
    step_px = base_unit_px * n_steps;

    /* As pages will be stiched together, we start drawing pretending that we are in the middle of
     * a "big tick" (which is step_px long) so we preload that amount of pixels. As we have assumed
     * starting with the "bigger half" (see comments in ladder-page.h), we need to preload the
     * "small half" has already written.
     *
     * We also preload the correct number of ticks within that range as if we'd drawn them, *plus*
     * the starting big tick of that interval we are in the middle of so everything will line up.
     */
    if(step_px % 2 == 0){
        preload_px = step_px/2;
    }else{
        preload_px = floor(step_px/2.0);
    }
    preload_ticks = preload_px / base_unit_px + 1;
//    printf("Preload pixels: %d, preload ticks: %d\n", preload_px, preload_ticks);

    surface = GENERIC_LAYER(self)->canvas;
    white = SDL_UWHITE(surface);

    SDL_LockSurface(surface);
    pixels = surface->pixels;
    ticks_drawn = preload_ticks;
    for(int y = surface->h-1, cnt = 1; y >= 0; y--, cnt++){
        lastx = location == LocationLeft ? 0 : (surface->w-1);
        firstx = lastx;
        if( (cnt + preload_px) % base_unit_px == 0){ /*tick mark*/
            if(ticks_drawn % n_steps == 0){ /*big tick mark*/
                if(location == LocationLeft)
                    lastx = firstx + big_step_width;
                else
                    firstx = lastx - big_step_width;
            }else{ /*small tick mark*/
                if(location == LocationLeft)
                    lastx = firstx + small_step_width;
                else
                    firstx = lastx - small_step_width;
            }
            ticks_drawn++;
        }else{
            firstx = lastx;
        }
        for(int j = firstx; j <= lastx; j++){
            Uint32 idx = y * surface->w + j;
            pixels[idx] = white;
        }
    }
    SDL_UnlockSurface(surface);
}

/*Put markings*/
void ladder_page_etch_markings(LadderPage *self, PCF_Font *font, uint halign, int offset)
{
    float y;

    VerticalStrip *strip;
    GenericLayer *layer;
    int page_index;

    assert((halign & HALIGN_RIGHT || halign & HALIGN_LEFT));

    strip = VERTICAL_STRIP(self);
    layer = GENERIC_LAYER(self);

    page_index = ladder_page_get_index(self);
//    printf("Page %d real range is [%f, %f]\n",page_index, strip->start, strip->end);
//
//    printf("Writing indexes on %d starting at %d to %f\n",page_index, page_index*self->descriptor->page_size, strip->end);
    Uint32 white = SDL_UWHITE(layer->canvas);
//    printf("Going from %zu to %f with step %f\n", page_index*self->descriptor->page_size, strip->end, self->descriptor->vstep);
    for(int i = page_index*self->descriptor->page_size; i <= strip->end; i += self->descriptor->vstep){
        y = ladder_page_resolve_value(self, i);
//        printf("Writing index %d at y = %f\n", i, y);
        int x;
        PCF_TextPlacement placement;
        if(halign & HALIGN_RIGHT){
            x = (generic_layer_w(layer)-1) - offset;
            placement = LeftToCol;
        }else{
            x = offset;
            placement = RightToCol;
        }
        PCF_FontWriteNumberAt(font,
            &i, TypeInt, 0,
            white, false, layer->canvas,
            x, y, placement | CenterOnRow);
    }
}
