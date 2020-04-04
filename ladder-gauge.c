#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "ladder-gauge.h"

//ms
#define SPIN_DURATION 1000


void ladder_page_free(LadderPage *self);

LadderGauge *ladder_gauge_new(ScrollType direction,  int rubis)
{
    LadderGauge *self;

    self = calloc(1, sizeof(LadderGauge));
    if(self){
        self->gauge = SDL_CreateRGBSurface(0, 68, 240, 32, 0, 0, 0, 0);
        SDL_SetColorKey(self->gauge, SDL_TRUE, SDL_MapRGB(self->gauge->format, 255, 0,255));
        self->damaged = true;
        self->direction = direction;
        if(rubis > 0)
            self->rubis = rubis;
        else
            self->rubis = round(self->gauge->h/2.0);
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
    SDL_FreeSurface(self->gauge);
    free(self);
}

LadderPage *ladder_page_new(float start, float end, float vstep, ScrollType direction)
{
    LadderPage *self;

    self = calloc(1, sizeof(LadderPage));
    if(self){
        self->fvo = 17;
        self->start = start;
        self->end = end;
        self->vstep = vstep;
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


    self->page = IMG_Load("alt-ladder.png");
    if(!self->page){
        return NULL;
    }
    self->ppv = self->page->h/(self->end - self->start +1);

    font = TTF_OpenFont("TerminusTTF-4.47.0.ttf", 18);
    SDL_Color white = (SDL_Color){255, 255, 255};

    for(int i = self->start; i <= self->end; i += self->vstep){
        snprintf(number, 6, "%d", i);
        text = TTF_RenderText_Solid(font, number, white);

        y = ladder_page_resolve_value(self, i);
        dst.y = y - text->h/2.0; /*verticaly center text*/
        dst.x = 10 + 5;

        SDL_BlitSurface(text, NULL, self->page, &dst);
        SDL_FreeSurface(text);
    }


    TTF_CloseFont(font);

    return self;
}

void ladder_page_free(LadderPage *self)
{
    if(self->page)
        SDL_FreeSurface(self->page);
    free(self);
}

bool ladder_page_has_value(LadderPage *self, float value)
{
    float min, max;

    min = (self->end > self->start) ? self->start : self->end;
    max = (self->end < self->start) ? self->start : self->end;

    return value >= min && value <= max;
}

/**
 * Returns image pixel index (i.e y for vertical gauges, x for horizontal)
 * from a given value
 * TODO be part of generic gauges layer
 */

float ladder_page_resolve_value(LadderPage *self, float value)
{
    float y;

    if(!ladder_page_has_value(self, value))
        return -1;

    value = fmod(value, fabs(self->end - self->start) + 1);

    y =  round(value * self->ppv + self->fvo);
    if(self->direction == BOTTUM_UP)
        y = (self->page->h-1) - y;

    return y;
}

int ladder_page_get_index(LadderPage *self)
{
    return self->start / PAGE_SIZE;
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
            printf("j = %d - %d = %d\n",i,offset,j);
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

void ladder_gauge_render_value(LadderGauge *self, float value, float rubis)
{
    float y;
    LadderPage *page, *page2;
    SDL_Rect dst_region = {0,0,0,0};

//    SDL_FillRect(self->gauge, NULL, 0x000000ff);
    SDL_FillRect(self->gauge, NULL, SDL_MapRGB(self->gauge->format, 255, 0,255));

    value = value >= 0 ? value : 0.0f;

    page = ladder_gauge_get_page_for(self, value);

    y = ladder_page_resolve_value(page, value);
    rubis = (rubis < 0) ? self->gauge->h / 2.0 : rubis;
    SDL_Rect portion = {
        .x = 0,
        .y = round(y - rubis),
        .w = page->page->w,
        .h = self->gauge->h
    };
    /*All pages must have the same size*/
    if(portion.y < 0){ //Fill top
        SDL_Rect patch = {
            .x = 0,
            .y = page->page->h + portion.y, //means - portion.y as portion.y < 0 here
            .w = page->page->w,
            .h = page->page->h - patch.y
        };
        if(self->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the top with values before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                SDL_BlitSurface(page2->page, &patch, self->gauge, &dst_region);
            }
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the top with values after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            SDL_BlitSurface(page2->page, &patch, self->gauge, &dst_region);

        }
        dst_region.y = patch.h;
        portion.y = 0;
        portion.h -= patch.h;
    }

    SDL_BlitSurface(page->page, &portion, self->gauge, &dst_region);

    if(portion.y + self->gauge->h > page->page->h){// fill bottom
        float taken = page->page->h - portion.y; //number of pixels taken from the bottom of values pict
        float delta = self->gauge->h - taken;
        dst_region.y += taken;
        SDL_Rect patch = {
            .x = 0,
            .y = 0,
            .w = page->page->w,
            .h = delta
        };
        if(self->direction == TOP_DOWN){
            /* 0 is on top, 100 is downwards. We need to fill the bottom with values that are after the end
             * of the current page, i.e get the next page */
            int pidx = ladder_page_get_index(page);
            page2 = ladder_gauge_get_page(self, pidx + 1);
            SDL_BlitSurface(page2->page, &patch, self->gauge, &dst_region);
        }else{
            /* 0 at the bottom, 100 is upwards. We need to fill the bottom with values that are before the begining
             * of the current page, i.e get the previous page */
            int pidx = ladder_page_get_index(page);
            if(pidx > 0){
                page2 = ladder_gauge_get_page(self, pidx - 1);
                SDL_BlitSurface(page2->page, &patch, self->gauge, &dst_region);
            }
        }
    }
}

void ladder_gauge_set_value(LadderGauge *self, float value)
{
    basic_animation_start(&self->animation, self->value, value, SPIN_DURATION);
    self->value = value;
}

static void ladder_gauge_draw_rubis(LadderGauge *self)
{
    SDL_LockSurface(self->gauge);
    Uint32 *pixels = self->gauge->pixels;
    Uint32 color = SDL_MapRGB(self->gauge->format, 0xFF, 0x00, 0x00);
    int empty_pixels = self->gauge->w / 2;
    int stop_idx = round(empty_pixels/2.0);
    int restart_idx = round(self->gauge->w - empty_pixels/2.0);
    for(int x = 0; x < self->gauge->w; x++){
        if(!empty_pixels || x < stop_idx || x >= restart_idx)
            pixels[self->rubis * self->gauge->w + x] = color;
    }
    SDL_UnlockSurface(self->gauge);
}


SDL_Surface *ladder_gauge_render(LadderGauge *self, uint32_t dt)
{
    float _current;

    if(self->animation.current != self->value || self->damaged){
        _current = basic_animation_loop(&self->animation, dt);

        ladder_gauge_render_value(self, _current, -1);
        ladder_gauge_draw_rubis(self);
        self->damaged = false;
    }
    return self->gauge;
}
