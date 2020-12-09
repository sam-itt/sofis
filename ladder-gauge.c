#include <stdio.h>
#include <stdlib.h>

#include "ladder-gauge.h"
#include "ladder-page-factory.h"
#include "generic-layer.h"
#include "sdl-colors.h"

static void ladder_gauge_update_state(LadderGauge *self, Uint32 dt);
static void ladder_gauge_render(LadderGauge *self, Uint32 dt, RenderContext *ctx);
static BaseGaugeOps ladder_gauge_ops = {
   .render = (RenderFunc)ladder_gauge_render,
   .update_state = (StateUpdateFunc)ladder_gauge_update_state
};


LadderGauge *ladder_gauge_new(LadderPageDescriptor *descriptor, int rubis)
{
    LadderGauge *self;

    self = calloc(1, sizeof(LadderGauge));
    if(self){
        if(!ladder_gauge_init(self, descriptor,rubis)){
            free(self);
            return NULL;
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


void ladder_gauge_free(LadderGauge *self)
{
    for(int i = 0; i < N_PAGES; i++){
        if(self->pages[i]){
            ladder_page_free(self->pages[i]);
            self->pages[i] = NULL;
        }
    }
    base_gauge_dispose(BASE_GAUGE(self));
    free(self->descriptor); /*No need for virtual dispose ATM*/
    free(self);
}


void ladder_gauge_set_value(LadderGauge *self, float value, bool animated)
{
    BaseAnimation *animation;

    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            animation = base_animation_new(TYPE_FLOAT, 1, &self->value);
            base_gauge_add_animation(BASE_GAUGE(self), animation);
            base_animation_unref(animation);/*base_gauge takes ownership*/
        }else{
            animation = BASE_GAUGE(self)->animations[0];
        }
        base_animation_start(animation, self->value, value, DEFAULT_DURATION);
    }else{
        self->value = value;
        BASE_GAUGE(self)->dirty = true;
    }
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

    self->value = self->value >= 0 ? self->value : 0.0f;

    page = ladder_gauge_get_page_for(self, self->value);

    y = ladder_page_resolve_value(page, self->value);
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
            .h = generic_layer_h(GENERIC_LAYER(page)) - patch.y
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
    self->state.rubis_y = self->rubis;
    self->state.pskip = round(base_gauge_w(BASE_GAUGE(self))/2.0);
}

static void ladder_gauge_render(LadderGauge *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);
    for(int i = 0; i < self->state.npatches; i++){
        base_gauge_blit_layer(BASE_GAUGE(self), ctx,
            self->state.patches[i].layer,
            &self->state.patches[i].src,
            &self->state.patches[i].dst
        );
    }
    base_gauge_draw_rubis(BASE_GAUGE(self),
        ctx, self->state.rubis_y,
        &SDL_RED, self->state.pskip
    );
}

#if 0
static void ladder_gauge_render_value(LadderGauge *self, float value)
{
    float y;
    float rubis;
    LadderPage *page, *page2;
    SDL_Rect dst_region = {0,0,BASE_GAUGE(self)->w,BASE_GAUGE(self)->h};

    buffered_gauge_clear(BUFFERED_GAUGE(self));
    buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, NULL);

    value = value >= 0 ? value : 0.0f;

    page = ladder_gauge_get_page_for(self, value);

    y = ladder_page_resolve_value(page, value);
//    printf("y = %f for value = %f\n",y,value);
    rubis = (self->rubis < 0) ? BASE_GAUGE(self)->h / 2.0 : self->rubis;
    SDL_Rect portion = {
        .x = 0,
        .y = round(y - rubis),
        .w = generic_layer_w(GENERIC_LAYER(page)),
        .h = BASE_GAUGE(self)->h
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
            .h = generic_layer_h(GENERIC_LAYER(page)) - patch.y
        };
        if(self->descriptor->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the top with values before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                buffered_gauge_blit_layer(BUFFERED_GAUGE(self), GENERIC_LAYER(page2), &patch, &dst_region);
            }
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the top with values after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            buffered_gauge_blit_layer(BUFFERED_GAUGE(self), GENERIC_LAYER(page2), &patch, &dst_region);
        }
        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }
    buffered_gauge_blit_layer(BUFFERED_GAUGE(self), GENERIC_LAYER(page), &portion, &dst_region);

    if(portion.y + BASE_GAUGE(self)->h > generic_layer_h(GENERIC_LAYER(page))){// fill bottom
        float taken = generic_layer_h(GENERIC_LAYER(page)) - portion.y; //number of pixels taken from the bottom of values pict
        float delta = BASE_GAUGE(self)->h - taken;
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
            buffered_gauge_blit_layer(BUFFERED_GAUGE(self), GENERIC_LAYER(page2), &patch, &dst_region);
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the bottom with values that are before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                buffered_gauge_blit_layer(BUFFERED_GAUGE(self), GENERIC_LAYER(page2), &patch, &dst_region);
            }
        }
    }
    buffered_gauge_draw_rubis(BUFFERED_GAUGE(self), self->rubis, &SDL_RED, round(BASE_GAUGE(self)->w/2.0));
}
#endif
