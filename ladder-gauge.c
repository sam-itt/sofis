#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "ladder-gauge.h"

//ms
#define SPIN_DURATION 1000


static void ladder_gauge_render_value(LadderGauge *self, float value);
void ladder_page_free(LadderPage *self);
static void ladder_gauge_draw_rubis(LadderGauge *self);

LadderGauge *ladder_gauge_new(ScrollType direction,  int rubis)
{
    LadderGauge *self;

    self = calloc(1, sizeof(LadderGauge));
    if(self){
        ANIMATED_GAUGE(self)->view = SDL_CreateRGBSurface(0, 68, 240, 32, 0, 0, 0, 0);
        SDL_SetColorKey(ANIMATED_GAUGE(self)->view, SDL_TRUE, SDL_MapRGB(ANIMATED_GAUGE(self)->view->format, 255, 0,255));
        ANIMATED_GAUGE(self)->damaged = true;
        ANIMATED_GAUGE(self)->renderer = (ValueRenderFunc)ladder_gauge_render_value;

        self->direction = direction;

        if(rubis > 0)
            self->rubis = rubis;
        else
            self->rubis = round(ANIMATED_GAUGE(self)->view->h/2.0);
    }

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
    free(self);
}

LadderPage *ladder_page_new(float start, float end, float vstep, ScrollType direction)
{
    LadderPage *self;

    self = calloc(1, sizeof(LadderPage));
    if(self){
        VERTICAL_STRIP(self)->fvo = 17;
        VERTICAL_STRIP(self)->start = start;
        VERTICAL_STRIP(self)->end = end;
        VERTICAL_STRIP(self)->vstep = vstep;
        self->direction = direction;

        if(!ladder_page_init(self)){
            free(self);
            return NULL;
        }


    }
    return self;
}

LadderPage *ladder_page_init(LadderPage *self)
{
    SDL_Surface *text;
    TTF_Font *font;
    SDL_Rect dst;
    char number[6]; //5 digits plus \0
    float y;
    VerticalStrip *strip;

    strip = VERTICAL_STRIP(self);
    strip->ruler = IMG_Load("alt-ladder.png");
    if(!strip->ruler){
        return NULL;
    }
    strip->ppv = strip->ruler->h/(strip->end - strip->start +1);

    font = TTF_OpenFont("TerminusTTF-4.47.0.ttf", 16);
    SDL_Color white = (SDL_Color){255, 255, 255};

    for(int i = strip->start; i <= strip->end; i += strip->vstep){
        snprintf(number, 6, "%d", i);
        text = TTF_RenderText_Solid(font, number, white);

        y = vertical_strip_resolve_value(strip, i, self->direction == BOTTUM_UP);
        dst.y = y - text->h/2.0; /*verticaly center text*/
        dst.x = 10 + 5;

        SDL_BlitSurface(text, NULL, strip->ruler, &dst);
        SDL_FreeSurface(text);
    }


    TTF_CloseFont(font);

    return self;
}

void ladder_page_free(LadderPage *self)
{
    vertical_strip_dispose(VERTICAL_STRIP(self));
    free(self);
}

int ladder_page_get_index(LadderPage *self)
{
    return VERTICAL_STRIP(self)->start / PAGE_SIZE;
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
        self->pages[a_idx] = ladder_page_new(idx * PAGE_SIZE, idx * PAGE_SIZE + (PAGE_SIZE-1), 100, self->direction);

    return self->pages[a_idx];
}


LadderPage *ladder_gauge_get_page_for(LadderGauge *self, float value)
{
    int page_idx;

    page_idx = (int)value/PAGE_SIZE;

    return ladder_gauge_get_page(self, page_idx);
}

void ladder_gauge_draw_outline(LadderGauge *self)
{
    int x,y;
    SDL_Surface *gauge;

    gauge = ANIMATED_GAUGE(self)->view;

    SDL_LockSurface(gauge);
    Uint32 *pixels = gauge->pixels;
    Uint32 color = SDL_MapRGB(gauge->format, 0xFF, 0xFF, 0xFF);
    y = 0;
    for(x = 0; x < gauge->w; x++){
        pixels[y * gauge->w + x] = color;
    }
    y = gauge->h - 1;
    for(x = 0; x < gauge->w; x++){
        pixels[y * gauge->w + x] = color;
    }
    x = gauge->w - 1;
    for(y = 0; y < gauge->h; y++){
        pixels[y * gauge->w + x] = color;
    }
    /*TODO: don't draw left side if current value > page 1 limit*/
    x = 0;
    for(y = 0; y < gauge->h; y++){
        pixels[y * gauge->w + x] = color;
    }

    SDL_UnlockSurface(gauge);
}

static void ladder_gauge_render_value(LadderGauge *self, float value)
{
    float y;
    float rubis;
    LadderPage *page, *page2;
    SDL_Rect dst_region = {0,0,0,0};
    SDL_Surface *gauge;

    gauge = ANIMATED_GAUGE(self)->view;

    SDL_FillRect(gauge, NULL, SDL_MapRGB(gauge->format, 255, 0,255));
    ladder_gauge_draw_outline(self);

    value = value >= 0 ? value : 0.0f;

    page = ladder_gauge_get_page_for(self, value);

    y = vertical_strip_resolve_value(VERTICAL_STRIP(page), value, self->direction == BOTTUM_UP);
    rubis = (self->rubis < 0) ? gauge->h / 2.0 : self->rubis;
    SDL_Rect portion = {
        .x = 0,
        .y = round(y - rubis),
        .w = VERTICAL_STRIP(page)->ruler->w,
        .h = gauge->h
    };
    /*All pages must have the same size*/
    if(portion.y < 0){ //Fill top
        SDL_Rect patch = {
            .x = 0,
            .y = VERTICAL_STRIP(page)->ruler->h + portion.y, //means - portion.y as portion.y < 0 here
            .w = VERTICAL_STRIP(page)->ruler->w,
            .h = VERTICAL_STRIP(page)->ruler->h - patch.y
        };
        if(self->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the top with values before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, gauge, &dst_region);
            }
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the top with values after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, gauge, &dst_region);

        }
        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }

    SDL_BlitSurface(VERTICAL_STRIP(page)->ruler, &portion, gauge, &dst_region);

    if(portion.y + gauge->h > VERTICAL_STRIP(page)->ruler->h){// fill bottom
        float taken = VERTICAL_STRIP(page)->ruler->h - portion.y; //number of pixels taken from the bottom of values pict
        float delta = gauge->h - taken;
        dst_region.y += taken;
        SDL_Rect patch = {
            .x = 0,
            .y = 0,
            .w = VERTICAL_STRIP(page)->ruler->w,
            .h = delta
        };
        if(self->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the bottom with values that are after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, gauge, &dst_region);
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the bottom with values that are before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                SDL_BlitSurface(VERTICAL_STRIP(page2)->ruler, &patch, gauge, &dst_region);
            }
        }
    }
    ladder_gauge_draw_rubis(self);
}

static void ladder_gauge_draw_rubis(LadderGauge *self)
{
    SDL_Surface *gauge;

    gauge = ANIMATED_GAUGE(self)->view;

    SDL_LockSurface(gauge);
    Uint32 *pixels = gauge->pixels;
    Uint32 color = SDL_MapRGB(gauge->format, 0xFF, 0x00, 0x00);
    int empty_pixels = gauge->w / 2;
    int stop_idx = round(empty_pixels/2.0);
    int restart_idx = round(gauge->w - empty_pixels/2.0);
    for(int x = 0; x < gauge->w; x++){
        if(!empty_pixels || x < stop_idx || x >= restart_idx)
            pixels[self->rubis * gauge->w + x] = color;
    }
    SDL_UnlockSurface(gauge);
}
