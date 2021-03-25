/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_gpu.h>

#include "elevator-gauge.h"
#include "misc.h"
#include "res-dirs.h"

static void elevator_gauge_render(ElevatorGauge *self, Uint32 dt, RenderContext *ctx);
static void elevator_gauge_update_state(ElevatorGauge *self, Uint32 dt);
static void *elevator_gauge_dispose(ElevatorGauge *self);
static BaseGaugeOps elevator_gauge_ops = {
   .render = (RenderFunc)elevator_gauge_render,
   .update_state = (StateUpdateFunc)elevator_gauge_update_state,
   .dispose = (DisposeFunc)elevator_gauge_dispose
};

static bool elevator_gauge_build_elevator(ElevatorGauge *self, Uint32 color);

/**
 * @brief Creates a new ElevatorGauge. Calling code is responsible
 * for the freeing.
 *
 * @see elevator_gauge_init for params and return value
 */
ElevatorGauge *elevator_gauge_new(bool marked, Location elevator_location,
                                  PCF_Font *font, SDL_Color color,
                                  float from, float to, float step,
                                  int bar_max_w, int bar_max_h,
                                  int nzones, ColorZone *zones)
{
    ElevatorGauge *self;
    bool rv;

    self = calloc(1, sizeof(ElevatorGauge));
    if(self){
        rv = elevator_gauge_init(self,
            marked, elevator_location,
            font, color,
            from, to, step,
            bar_max_w, bar_max_h,
            nzones, zones);
        if(!rv){
            return base_gauge_dispose(BASE_GAUGE(self));
        }
    }
    return self;
}

/**
 * @brief Inits a ElevatorGauge
 *
 * @param self a ElevatorGauge
 * @param marked Write values corresponding to hatch marks
 * @param location Which side (left or right) to put the elevator relative
 * to the ruler.
 * @param font The font to use for markings
 * @param color the color to use when writing hatch marks and markings
 * @param from start of the value range
 * @param to end of the value range
 * @param step increment in value units (meters, degrees, etc.) of hatch
 * marks. Passing in a negative value will create only hatch marks at the
 * begining/end of the range. See generic_ruler_init for a discussion of
 * valid @p step values.
 * @param bar_max_w Maximum width of the bar itself. It can be shrinked to
 * accomodate the needed number of hatch marks.
 * @param bar_max_h Maximum height of the bar itself. The gauge will be
 * larger to accomadate markings (if any) and cursor(s).
 * @param nzones size of the @p zones array, 0 if none
 * @param zones array of ColorZones that will be used in the gauge. NULL
 * for none.
 * @return @p self on success, NULL on failure.
 *
 * @note Array is copied, caller can pass in a temporary local array.
 *
 * @see generic_ruler_init
 * @see elevator_gauge_new
 */
ElevatorGauge *elevator_gauge_init(ElevatorGauge *self,
                                   bool marked, Location elevator_location,
                                   PCF_Font *font, SDL_Color color,
                                   float from, float to, float step,
                                   int bar_max_w, int bar_max_h,
                                   int nzones, ColorZone *zones)
{
    Location marks_location;
    Location spine_location;

    self->elevator_location = elevator_location;
    marks_location = (self->elevator_location == Left) ? Right : Left;
    spine_location = elevator_location;

    generic_ruler_init(&(self->ruler),
        RulerVertical, RulerGrowAgainstAxis,
        from, to, step,
        font, marks_location, 0,
        bar_max_w, bar_max_h
    );
    SFV_GAUGE(self)->value = from;

    self->nzones = nzones;
    self->zones = calloc(self->nzones, sizeof(ColorZone));
    if(!self->zones)
        return NULL;

    for(int i = 0; i < nzones; i++){
        self->zones[i] = zones[i];
    }

    Uint32 fcolor;
    fcolor =  SDL_MapRGBA(
        GENERIC_LAYER(&self->ruler)->canvas->format,
        color.r, color.g, color.b, color.a
    );

    /*Draws the ruler*/
    bool rv;
    if(self->nzones > 0){
        bool rv = generic_ruler_draw_zones(&self->ruler, spine_location, self->nzones, self->zones, 0.7);
        if(!rv)
            printf("Draw zones failed!\n");
    }
    rv = generic_ruler_etch_hatches(&(self->ruler), fcolor, false, true, marks_location);
    if(!rv)
        printf("Draw etches failed!\n");

    if(marked && font){ /*Font will also be used to tag the cursors (itf)*/
        rv = generic_ruler_etch_markings(&(self->ruler), marks_location, font, fcolor, 0);
        if(!rv)
            printf("Draw markings failed!\n");
    }
    generic_layer_build_texture(GENERIC_LAYER(&self->ruler));

    elevator_gauge_build_elevator(self, fcolor);
    if(!self->elevator)
        return false;

    self->ruler_rect = (SDL_Rect){
        .x = self->elevator->canvas->w,
        .y = 0,
        .w = GENERIC_LAYER(&self->ruler)->canvas->w,
        .h = GENERIC_LAYER(&self->ruler)->canvas->h
    };

    base_gauge_init(BASE_GAUGE(self),
        &elevator_gauge_ops,
        GENERIC_LAYER(&self->ruler)->canvas->w + self->elevator->canvas->w,
        GENERIC_LAYER(&self->ruler)->canvas->h
    );

    return self;
}

/**
 * @brief Release resources held by @p self.
 *
 * @param self a ElevatorGauge
 */
static void *elevator_gauge_dispose(ElevatorGauge *self)
{
    generic_ruler_dispose(&self->ruler);
    if(self->elevator)
        generic_layer_free(self->elevator);
    if(self->zones)
        free(self->zones);

    return self;
}

bool elevator_gauge_set_value(ElevatorGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;

    generic_ruler_clip_value(&self->ruler, &value);

    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}

/*
 * @brief Creates the elevator bitmap
 *
 * INTERNAL USE ONLY
 *
 * TODO: Generate the cursor otf without using files
 */
static bool elevator_gauge_build_elevator(ElevatorGauge *self, Uint32 color)
{
    char *filename;
    int startx, endx;

    if(self->elevator_location != Left && self->elevator_location != Right){
        printf("Unsupported location\n");
        return false;
    }

    filename = (self->elevator_location == Left) ? IMG_DIR"/lh-cursor10.png" : IMG_DIR"/rh-cursor10.png";

    SDL_Surface *triangle = IMG_Load(filename);
    if(!triangle)
        return NULL;
    SDL_SetSurfaceBlendMode(triangle, SDL_BLENDMODE_NONE);

    self->elevator = generic_layer_new(triangle->w, self->ruler.ruler_area.h);
    if(!self->elevator)
        return NULL;

    /*Blit triangle at top left of elevator*/
    SDL_BlitSurface(triangle, NULL, self->elevator->canvas, NULL);
    SDL_FreeSurface(triangle);
    /*Creates the 'tail'*/
    startx = (self->elevator_location == Left)
             ? 0
             : self->elevator->canvas->w - 4;
    endx = (self->elevator_location == Left)
           ? 4
           : self->elevator->canvas->w;
    generic_layer_lock(self->elevator);
    Uint32 *pixels = self->elevator->canvas->pixels;
    for(int y = 4; y < self->elevator->canvas->h; y++){
        for(int x = startx; x < endx; x++)
            pixels[y * self->elevator->canvas->w + x] = color;
    }
    generic_layer_unlock(self->elevator);

    generic_layer_build_texture(self->elevator);

    return true;
}

static void elevator_gauge_update_state(ElevatorGauge *self, Uint32 dt)
{
    int yinc;
    int elevator_top;

    yinc = generic_ruler_get_pixel_increment_for(&self->ruler, SFV_GAUGE(self)->value);
    elevator_top = SDLExt_RectLastY(&self->ruler.ruler_area) - yinc;

    /*Area to copy from the whole elevator image*/
    self->state.elevator_src = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = self->elevator->canvas->w,
        .h = yinc + 1
    };
    /*Where to put it*/
    self->state.elevator_dst = (SDL_Rect){
        .x = (self->elevator_location == Left)
             ? 0
             : SDLExt_RectLastX(&self->ruler_rect) + 1,
        .y = self->ruler_rect.y + elevator_top,
        .w = self->state.elevator_src.w,
        .h = self->state.elevator_src.h
    };

}

static void elevator_gauge_render(ElevatorGauge *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        GENERIC_LAYER(&self->ruler),
        NULL,
        &self->ruler_rect
    );

    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        self->elevator,
        &self->state.elevator_src,
        &self->state.elevator_dst
    );
}
