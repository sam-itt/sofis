#include <stdio.h>
#include <stdlib.h>

#include "animated-gauge.h"
#include "buffered-gauge.h"
#include "ladder-gauge.h"
#include "ladder-page-factory.h"
#include "sdl-colors.h"

static void ladder_gauge_render_value(LadderGauge *self, float value);
static AnimatedGaugeOps ladder_gauge_ops = {
   .render_value = (ValueRenderFunc)ladder_gauge_render_value
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
    animated_gauge_init(ANIMATED_GAUGE(self), ANIMATED_GAUGE_OPS(&ladder_gauge_ops), 68, 240);

    self->descriptor = descriptor;
    if(rubis > 0)
        self->rubis = rubis;
    else
        self->rubis = round(BASE_GAUGE(self)->h/2.0);
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
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    free(self->descriptor); /*No need for virtual dispose ATM*/
    free(self);
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

    page_idx = (int)value/(self->descriptor->page_size+self->descriptor->offset);

    return ladder_gauge_get_page(self, page_idx);
}

static void ladder_gauge_render_value(LadderGauge *self, float value)
{
    float y;
    float rubis;
    LadderPage *page, *page2;
    SDL_Rect dst_region = {0,0,0,0};
    SDL_Surface *tview;

    tview = buffered_gauge_get_view(BUFFERED_GAUGE(self));

    buffered_gauge_clear(BUFFERED_GAUGE(self));
    view_draw_outline(tview, &(SDL_WHITE), NULL);

    value = value >= 0 ? value : 0.0f;

    page = ladder_gauge_get_page_for(self, value);

    y = ladder_page_resolve_value(page, value);
//    printf("y = %f for value = %f\n",y,value);
    rubis = (self->rubis < 0) ? BASE_GAUGE(self)->h / 2.0 : self->rubis;
    SDL_Rect portion = {
        .x = 0,
        .y = round(y - rubis),
        .w = VERTICAL_STRIP(page)->ruler->w,
        .h = BASE_GAUGE(self)->h
    };
    /*All pages must have the same size*/
    if(portion.y < 0){ //Fill top
        SDL_Rect patch = {
            .x = 0,
            .y = VERTICAL_STRIP(page)->ruler->h + portion.y, //means - portion.y as portion.y < 0 here
            .w = VERTICAL_STRIP(page)->ruler->w,
            .h = VERTICAL_STRIP(page)->ruler->h - patch.y
        };
        if(self->descriptor->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the top with values before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, tview, &dst_region);
            }
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the top with values after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, tview, &dst_region);

        }
        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }

    SDL_BlitSurface(VERTICAL_STRIP(page)->ruler, &portion, tview, &dst_region);

    if(portion.y + BASE_GAUGE(self)->h > VERTICAL_STRIP(page)->ruler->h){// fill bottom
        float taken = VERTICAL_STRIP(page)->ruler->h - portion.y; //number of pixels taken from the bottom of values pict
        float delta = BASE_GAUGE(self)->h - taken;
        dst_region.y += taken;
        SDL_Rect patch = {
            .x = 0,
            .y = 0,
            .w = VERTICAL_STRIP(page)->ruler->w,
            .h = delta
        };
        if(self->descriptor->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the bottom with values that are after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, tview, &dst_region);
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the bottom with values that are before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, tview, &dst_region);
            }
        }
    }
    view_draw_rubis(tview, self->rubis, &SDL_RED, round(BASE_GAUGE(self)->w/2.0), NULL);
}
