/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "ladder-gauge.h"
#include "ladder-page-factory.h"
#include "generic-layer.h"
#include "sdl-colors.h"

static void ladder_gauge_update_state(LadderGauge *self, Uint32 dt);
static void ladder_gauge_render(LadderGauge *self, Uint32 dt, RenderContext *ctx);
static void *ladder_gauge_dispose(LadderGauge *self);
static BaseGaugeOps ladder_gauge_ops = {
   .render = (RenderFunc)ladder_gauge_render,
   .update_state = (StateUpdateFunc)ladder_gauge_update_state,
   .dispose = (DisposeFunc)ladder_gauge_dispose
};


LadderGauge *ladder_gauge_new(LadderPageDescriptor *descriptor, int rubis)
{
    LadderGauge *self;

    self = calloc(1, sizeof(LadderGauge));
    if(self){
        if(!ladder_gauge_init(self, descriptor,rubis)){
            return base_gauge_free(BASE_GAUGE(self));
        }
    }
    return self;
}

LadderGauge *ladder_gauge_init(LadderGauge *self, LadderPageDescriptor *descriptor, int rubis)
{
    base_gauge_init(BASE_GAUGE(self), &ladder_gauge_ops, 68, 240);

    self->descriptor = descriptor;
    if(rubis > 0)
        self->rubis = rubis;
    else
        self->rubis = round(base_gauge_h(BASE_GAUGE(self))/2.0);
    return self;
}

static void *ladder_gauge_dispose(LadderGauge *self)
{
    for(int i = 0; i < N_PAGES; i++){
        if(self->pages[i]){
            ladder_page_free(self->pages[i]);
            self->pages[i] = NULL;
        }
    }
    if(self->descriptor)
        free(self->descriptor); /*No need for virtual dispose ATM*/

    return self;
}

bool ladder_gauge_set_value(LadderGauge *self, float value, bool animated)
{
    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}

/**
 *
 * @param idx: the page number, computed from page range.
 * If range is 0-699, then page 0 would be 0 -> 699, page 1
 * 700-1399, etc.
 */
LadderPage *ladder_gauge_get_page(LadderGauge *self, uintf8_t idx)
{
    int a_idx;
    uintf8_t cmp; /*current max page*/
    uintf8_t offset; /*Number of pages to move (advance of back up)*/
    int j;

    cmp = self->base + (N_PAGES-1);
    if(idx > cmp){
        offset = idx - cmp;
        for(int i = 0; i < N_PAGES; i++){
            j = i - offset;
//            printf("j = %d - %d = %d\n",i,offset,j);
            if( j < 0 ){
                if(self->pages[i]){
                    ladder_page_free(self->pages[i]);
                    self->pages[i] = NULL;
                }
            }else{
                self->pages[j] = self->pages[i];
                self->pages[i] = NULL;
            }
        }
        self->base += offset;
    }else if(idx < self->base){
        offset = self->base - idx;
         for(int i = N_PAGES-1; i >= 0; i--){
            j = i + offset;
            if( j > N_PAGES-1){
                if(self->pages[i]){
                    ladder_page_free(self->pages[i]);
                    self->pages[i] = NULL;
                }
            }else{
                self->pages[j] = self->pages[i];
                self->pages[i] = NULL;
            }
        }
        self->base -= offset;
    }

    a_idx = idx - self->base;
    if(!self->pages[a_idx])
        self->pages[a_idx] = ladder_page_factory_create(idx, self->descriptor);

    return self->pages[a_idx];
}

LadderPage *ladder_gauge_get_page_for(LadderGauge *self, float value)
{
    int page_idx;

    page_idx = (int)value/(self->descriptor->page_size);

    return ladder_gauge_get_page(self, page_idx);
}

static void ladder_gauge_update_state(LadderGauge *self, Uint32 dt)
{
    float y;
    float rubis;
    LadderPage *page, *page2;
    SDL_Rect dst_region = {0,0,base_gauge_w(BASE_GAUGE(self)),base_gauge_h(BASE_GAUGE(self))};

    memset(&self->state, 0, sizeof(LadderGaugeState));

    SFV_GAUGE(self)->value = SFV_GAUGE(self)->value >= 0 ? SFV_GAUGE(self)->value : 0.0f;

    page = ladder_gauge_get_page_for(self, SFV_GAUGE(self)->value);

    y = ladder_page_resolve_value(page, SFV_GAUGE(self)->value);
//    printf("y = %f for value = %f\n",y,value);
    rubis = (self->rubis < 0) ? base_gauge_h(BASE_GAUGE(self)) / 2.0 : self->rubis;
    SDL_Rect portion = {
        .x = 0,
        .y = round(y - rubis),
        .w = generic_layer_w(GENERIC_LAYER(page)),
        .h = base_gauge_h(BASE_GAUGE(self))
    };
    /* Ensures that portion.y + portion.h doesn't got past image bounds:
     * w/h are ignored by SDL_BlitSurface, but when using SDL_Renderers wrong
     * values will stretch the image.
     * */
    if(portion.y + portion.h > generic_layer_h(GENERIC_LAYER(page)))
        portion.h = generic_layer_h(GENERIC_LAYER(page)) - portion.y;
    /*All pages must have the same size*/
    if(portion.y < 0){ //Fill top
        SDL_Rect patch = {
            .x = 0,
            .y = generic_layer_h(GENERIC_LAYER(page)) + portion.y, //means - portion.y as portion.y < 0 here
            .w = generic_layer_w(GENERIC_LAYER(page)),
            .h = generic_layer_h(GENERIC_LAYER(page))
        };
        if(self->descriptor->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the top with values before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                self->state.patches[self->state.npatches].layer = GENERIC_LAYER(page2);
                self->state.patches[self->state.npatches].src = patch;
                self->state.patches[self->state.npatches].dst = dst_region;
                self->state.npatches++;
            }
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the top with values after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            self->state.patches[self->state.npatches].layer = GENERIC_LAYER(page2);
            self->state.patches[self->state.npatches].src = patch;
            self->state.patches[self->state.npatches].dst = dst_region;
            self->state.npatches++;
        }
        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }
    self->state.patches[self->state.npatches].layer = GENERIC_LAYER(page);
    self->state.patches[self->state.npatches].src = portion;
    self->state.patches[self->state.npatches].dst = dst_region;
    self->state.npatches++;

    if(portion.y + base_gauge_h(BASE_GAUGE(self)) > generic_layer_h(GENERIC_LAYER(page))){// fill bottom
        float taken = generic_layer_h(GENERIC_LAYER(page)) - portion.y; //number of pixels taken from the bottom of values pict
        float delta = base_gauge_h(BASE_GAUGE(self)) - taken;
        dst_region.y += taken;
        SDL_Rect patch = {
            .x = 0,
            .y = 0,
            .w = generic_layer_w(GENERIC_LAYER(page)),
            .h = delta
        };
        if(self->descriptor->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the bottom with values that are after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            self->state.patches[self->state.npatches].layer = GENERIC_LAYER(page2);
            self->state.patches[self->state.npatches].src = patch;
            self->state.patches[self->state.npatches].dst = dst_region;
            self->state.npatches++;
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the bottom with values that are before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                self->state.patches[self->state.npatches].layer = GENERIC_LAYER(page2);
                self->state.patches[self->state.npatches].src = patch;
                self->state.patches[self->state.npatches].dst = dst_region;
                self->state.npatches++;
            }
        }
    }
    self->state.pskip = round(base_gauge_w(BASE_GAUGE(self))/2.0);
}

static void ladder_gauge_render(LadderGauge *self, Uint32 dt, RenderContext *ctx)
{
    for(int i = 0; i < self->state.npatches; i++){
        base_gauge_blit_layer(BASE_GAUGE(self), ctx,
            self->state.patches[i].layer,
            &self->state.patches[i].src,
            &self->state.patches[i].dst
        );
    }
    base_gauge_draw_rubis(BASE_GAUGE(self),
        ctx, self->rubis,
        &SDL_RED, self->state.pskip
    );
    base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);
}
