#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "animated-gauge.h"
#include "vertical-stair.h"
#include "sdl-colors.h"


static void vertical_stair_render_value(VerticalStair *self, float value);
static void vertical_stair_render_value_to(VerticalStair *self, float value, SDL_Surface *destination, SDL_Rect *location);
static AnimatedGaugeOps vertical_stair_ops = {
    .render_value = (ValueRenderFunc)vertical_stair_render_value,
    .render_value_to = (ValueRenderToFunc)vertical_stair_render_value_to
};


VerticalStairCursor *vertical_stair_cursor_init(VerticalStairCursor *self, const char *filename, int font_size)
{
    self->bg = IMG_Load(filename);
    self->value = NAN;
    self->font  = TTF_OpenFont("TerminusTTF-4.47.0.ttf", font_size);
    if(!self->bg || !self->font)
        return NULL;
    return self;
}

void vertical_stair_cursor_dispose(VerticalStairCursor *self)
{
    if(self->bg)
        SDL_FreeSurface(self->bg);
    if(self->font)
        TTF_CloseFont(self->font);
}

void vertical_stair_cursor_set_value(VerticalStairCursor *self, float value)
{
    SDL_Surface *text;
    SDL_Rect dst;
    char number[6]; //5 digits plus \0
    int ivalue;

    if(self->value != value){
        Uint32 black = SDL_UBLACK(self->bg);
        SDL_FillRect(self->bg, &(SDL_Rect){5,5,8,7}, black);
        SDL_FillRect(self->bg, &(SDL_Rect){13,0,self->bg->w-13,self->bg->h}, black);

        ivalue = round(value);
        snprintf(number, 6, "% -d", ivalue);
        text = TTF_RenderText_Solid(self->font, number, SDL_WHITE);

        dst = (SDL_Rect){7,-1,0,0};
        SDL_BlitSurface(text, NULL, self->bg, &dst);
        SDL_FreeSurface(text);

        self->value = value;
    }

}


VerticalStair *vertical_stair_new(const char *bg_img, const char *cursor_img, int cfont_size)
{
    VerticalStair *self;

    self = calloc(1, sizeof(VerticalStair));
    if(self){
        if(!vertical_stair_init(self, bg_img, cursor_img, cfont_size)){
            free(self);
            return NULL;
        }
    }
    return self;
}

/*TODO: Decouple VerticalStair to VerticalSpeedIndicator*/
VerticalStair *vertical_stair_init(VerticalStair *self, const char *bg_img, const char *cursor_img, int cfont_size)
{
    self->scale.ruler = IMG_Load(bg_img);
    self->scale.ppv = 43.0/1000.0; /*0.0426*/
    self->scale.start = -2279;
    self->scale.end = 2279;

    vertical_stair_cursor_init(&self->cursor, cursor_img, cfont_size);
    animated_gauge_init(ANIMATED_GAUGE(self), ANIMATED_GAUGE_OPS(&vertical_stair_ops), self->cursor.bg->w, self->scale.ruler->h);

    return self;
}

void vertical_stair_free(VerticalStair *self)
{
    animated_gauge_dispose(ANIMATED_GAUGE(self));
    vertical_strip_dispose(&self->scale);
    vertical_stair_cursor_dispose(&self->cursor);
    free(self);
}

static void vertical_stair_render_value(VerticalStair *self, float value)
{
    float y;

    vertical_stair_cursor_set_value(&self->cursor, value);
    vertical_strip_clip_value(&self->scale, &value);
    SDL_FillRect(ANIMATED_GAUGE(self)->view, NULL, SDL_UCKEY(ANIMATED_GAUGE(self)->view));
    SDL_BlitSurface(self->scale.ruler, NULL, ANIMATED_GAUGE(self)->view, NULL);

    y = vertical_strip_resolve_value(&self->scale, value, true);
    y = round(round(y) - self->cursor.bg->h/2.0);

    SDL_BlitSurface(self->cursor.bg, NULL, ANIMATED_GAUGE(self)->view, &(SDL_Rect){1, y,0,0});

}

static void vertical_stair_render_value_to(VerticalStair *self, float value, SDL_Surface *destination, SDL_Rect *location)
{
    float y;
    SDL_Rect cloc, rloc, area;
    int y_offset;

    y_offset = round(self->cursor.bg->h/2.0);

    rloc = (SDL_Rect){
        .x = location->x,
        .y = location->y + y_offset,
        /*These two are ignored but let's have the
         * code ready for SDL_Renderer transition*/
        /* VerticalStair as wide enough to accomodate the stair AND
         * the cursor which protudes to the right. We don't want the
         * ruler to be widened to keep the width the same as the image
         * */
        .w = self->scale.ruler->w,
        .h = BASE_GAUGE(self)->h
    };

    area = (SDL_Rect){
        location->x, location->y,
        BASE_GAUGE(self)->w,BASE_GAUGE(self)->h
    };

    vertical_stair_cursor_set_value(&self->cursor, value);
    vertical_strip_clip_value(&self->scale, &value);

    /*Clear the area before drawing. TODO, move upper in animated_gauge ?*/
    //SDL_FillRect(destination, &area, SDL_UFBLUE(destination));

    SDL_BlitSurface(self->scale.ruler, NULL, destination, &rloc);

    y = vertical_strip_resolve_value(&self->scale, value, true);
    y = round(round(y) - self->cursor.bg->h/2.0);
    y += y_offset;

    cloc = (SDL_Rect){
        .x = location->x + 1,
        .y = location->y + y,
        .w = 0,
        .h = 0
    };

    SDL_BlitSurface(self->cursor.bg, NULL, destination, &cloc);
}
