/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>

#include "SDL_render.h"
#include "airspeed-page-descriptor.h"
#include "generic-layer.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"
#include "vertical-strip.h"
#include "res-dirs.h"

#define PAGE_SIZE 70


LadderPage *airspeed_ladder_page_init(LadderPage *self);
static void airspeed_ladder_page_draw_arc(LadderPage *self, float start, float end, uintf8_t width, Uint32 color);
static void airspeed_ladder_page_draw_arcs(LadderPage *self);

AirspeedPageDescriptor *airspeed_page_descriptor_new(speed_t v_so, speed_t v_s1, speed_t v_fe, speed_t v_no, speed_t v_ne)
{
    AirspeedPageDescriptor *self;

    self = calloc(1, sizeof(AirspeedPageDescriptor));
    if(!self)
        return NULL;

    self->v_so = v_so;
    self->v_s1 = v_s1;
    self->v_fe = v_fe;
    self->v_no = v_no;
    self->v_ne = v_ne;

    fb_page_descriptor_init((FBPageDescriptor *)self, IMG_DIR"/speed-ladder.png", BOTTUM_UP, PAGE_SIZE, 10, 5);
    self->super.super.init_page = airspeed_ladder_page_init;
    self->super.super.fei = 234;

    return self;
}


LadderPage *airspeed_ladder_page_init(LadderPage *self)
{

    fb_ladder_page_init(self);
    ladder_page_etch_markings(self, resource_manager_get_font(TERMINUS_16));

    airspeed_ladder_page_draw_arcs(self);
    generic_layer_build_texture(GENERIC_LAYER(self));

    return self;
}

void airspeed_ladder_page_draw_arcs(LadderPage *self)
{
    AirspeedPageDescriptor *descriptor;
    float green_start, green_end;
    float yellow_start, yellow_end;
    float white_start, white_end;

    descriptor = (AirspeedPageDescriptor *)self->descriptor;
    green_start = (descriptor->v_s1);
    green_end = (descriptor->v_no);
    yellow_start = (descriptor->v_no);
    yellow_end = (descriptor->v_ne);
    white_start = (descriptor->v_so);
    white_end = (descriptor->v_fe);

    /*Green arc*/
//    printf("Green arc from %d kts to %d kts\n",descriptor->v_s1, descriptor->v_no);
    airspeed_ladder_page_draw_arc(self, descriptor->v_s1, descriptor->v_no, 4, SDL_UGREEN(GENERIC_LAYER(self)->canvas));
    /*Yellow*/
    airspeed_ladder_page_draw_arc(self, descriptor->v_no, descriptor->v_ne, 4, SDL_UYELLOW(GENERIC_LAYER(self)->canvas));
    /*White last, over any existing*/
    airspeed_ladder_page_draw_arc(self, descriptor->v_so, descriptor->v_fe, 2, SDL_UGREY(GENERIC_LAYER(self)->canvas));

    /*VNE*/
    airspeed_ladder_page_draw_arc(self, descriptor->v_ne, descriptor->v_ne+2, 4, SDL_URED(GENERIC_LAYER(self)->canvas));
}


static void airspeed_ladder_page_draw_arc(LadderPage *self, float start, float end, uintf8_t width, Uint32 color)
{
    float istart, iend;
    float ystart, yend;
    int firsty, lasty;
    Uint32 *pixels;
    SDL_Surface *surface;
    int x;
    Uint32 white;

    if(interval_intersect(start, end, VERTICAL_STRIP(self)->start, VERTICAL_STRIP(self)->end, &istart, &iend)){
        surface = GENERIC_LAYER(self)->canvas;
        white = SDL_UWHITE(surface);

        ystart = ladder_page_resolve_value(self, istart);
        yend = ladder_page_resolve_value(self, iend);

        firsty = (ystart < yend) ? ystart : yend;
        lasty = (yend > ystart) ? yend : ystart;

        if(firsty < 0) firsty = 0;
        if(lasty > (surface->h-1)) lasty = surface->h-1;
/*
        printf("Intersection between arc from %f to %f and current page from %f to %f is %f,%f and will be starting at Y:%d to Y:%d\n",
                start, end, VERTICAL_STRIP(self)->start, VERTICAL_STRIP(self)->end,
                istart, iend,
                firsty, lasty
                );*/

        SDL_LockSurface(surface);
        pixels = surface->pixels;
        for(int i = firsty; i <= lasty; i++){
            x = ((surface->w-1)-1) - (width-1); /*second -1 to avoid eating the border*/
            for(int j = x; j < surface->w; j++){
                Uint32 idx = i * surface->w + j;
                if(pixels[idx] != white) /*Avoid eating the marks*/
                    pixels[idx] = color;
            }
        }
        SDL_UnlockSurface(surface);
    }else{/*
        printf("No intersection between arc from %f to %f and current page from %f to %f\n",
                start, end, VERTICAL_STRIP(self)->start, VERTICAL_STRIP(self)->end
                );*/

    }
}

