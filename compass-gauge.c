#include <stdio.h>
#include <stdlib.h>

#include "base-gauge.h"
#include "compass-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "misc.h"
#include "text-gauge.h"

static void compass_gauge_render(CompassGauge *self, Uint32 dt, RenderContext *ctx);
static void compass_gauge_update_state(CompassGauge *self, Uint32 dt);
static BaseGaugeOps compass_gauge_ops = {
   .render = (RenderFunc)compass_gauge_render,
   .update_state = (StateUpdateFunc)compass_gauge_update_state
};


CompassGauge *compass_gauge_new(void)
{
    CompassGauge *rv;
    rv = calloc(1, sizeof(CompassGauge));
    if(rv){
        if(!compass_gauge_init(rv)){
            free(rv);
            return NULL;
        }
    }
    return(rv);
}

CompassGauge *compass_gauge_init(CompassGauge *self)
{
    bool rv;

    rv = generic_layer_init_from_file(&self->outer, "compass-outer.png");
    if(!rv){
        printf("Couldn't load compass outer ring\n");
        return NULL;
    }
    rv = generic_layer_init_from_file(&self->inner, "compass-inner.png");
    if(!rv){
        printf("Couldn't load compass inner ring\n");
        return NULL;
    }

    self->caption = text_gauge_new("000°", true, 28, 12);
    if(!self->caption)
        return NULL;
    text_gauge_set_color(self->caption, SDL_BLACK, BACKGROUND_COLOR);
    self->caption->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->caption,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_DIGITS, "°"
        )
    );

    base_gauge_init(BASE_GAUGE(self),
        &compass_gauge_ops,
        generic_layer_w(&self->outer),
        generic_layer_h(&self->outer) + base_gauge_h(BASE_GAUGE(self->caption))
    );

    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->caption),
        (int)SDLExt_RectMidX(&BASE_GAUGE(self)->state.frame)
        - (int)SDLExt_RectMidX(&BASE_GAUGE(self->caption)->state.frame),
        0
    );

    self->icenter = (SDL_Point){
        .x = (generic_layer_w(&self->inner) - 1)/2,
        .y = (generic_layer_h(&self->inner) - 1)/2,
    };

    self->outer_rect = (SDL_Rect){
        .x = 0,
        .y = base_gauge_h(BASE_GAUGE(self->caption)),
        .w = generic_layer_w(&self->outer),
        .h = generic_layer_h(&self->outer)
    };

    self->inner_rect = (SDL_Rect){
        .x = (generic_layer_w(&self->outer) - 1)/2 - self->icenter.x,
        .y = (generic_layer_h(&self->outer) - 1)/2 - self->icenter.y + self->outer_rect.y,
        .w = generic_layer_w(&self->inner),
        .h = generic_layer_h(&self->inner)
    };

    generic_layer_build_texture(&self->outer);
    generic_layer_build_texture(&self->inner);

#if !USE_SDL_GPU
	self->state.rbuffer = SDL_CreateRGBSurfaceWithFormat(0,
        base_gauge_w(BASE_GAUGE(self)),
        base_gauge_h(BASE_GAUGE(self)),
        32, SDL_PIXELFORMAT_RGBA32
    );
	self->renderer =  SDL_CreateSoftwareRenderer(self->state.rbuffer);
#endif
    return self;
}

void compass_gauge_dispose(CompassGauge *self)
{
    generic_layer_dispose(&self->outer);
    generic_layer_dispose(&self->inner);
    text_gauge_free(self->caption);
    base_gauge_dispose(BASE_GAUGE(self));
}

void compass_gauge_free(CompassGauge *self)
{
    compass_gauge_dispose(self);
    free(self);
}

bool compass_gauge_set_value(CompassGauge *self, float value, bool animated)
{
    bool rv = true;
    BaseAnimation *animation;

    value = fmod(value, 360.0);
    if(value < 0)
        value += 360.0;

    return sfv_gauge_set_value(SFV_GAUGE(self), value, animated);
}

static void compass_gauge_update_state(CompassGauge *self, Uint32 dt)
{
#if !USE_SDL_GPU
	if(SFV_GAUGE(self)->value != 0){
        SDL_Texture *tex = SDL_CreateTextureFromSurface(self->renderer, self->inner.canvas);
        SDL_RenderCopyEx(self->renderer, tex,
            NULL, NULL,
            SFV_GAUGE(self)->value * -1.0f,
            &self->icenter, SDL_FLIP_NONE
        );
		SDL_DestroyTexture(tex);
	}else{
        SDL_BlitSurface(self->inner.canvas, NULL,self->state.rbuffer, NULL);
    }
#endif
    char cvalue[5]; //3 digits, degree sign and '\0'
    snprintf(cvalue, 5, "%03d", (int)SFV_GAUGE(self)->value);
    text_gauge_set_value(self->caption, cvalue);
}

static void compass_gauge_render(CompassGauge *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_blit_layer(BASE_GAUGE(self), ctx,
        &self->outer,
        NULL, &self->outer_rect);
#if USE_SDL_GPU
    base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
        self->inner.texture,
        NULL,
        SFV_GAUGE(self)->value * -1.0f,
        &self->icenter,
        &self->inner_rect,
        NULL);
#endif
}

