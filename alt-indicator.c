#include <stdio.h>
#include <stdlib.h>

#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "alt-indicator.h"
#include "alt-ladder-page-descriptor.h"
#include "animated-gauge.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "misc.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "SDL_pcf.h"
#include "text-gauge.h"

static void alt_indicator_render(AltIndicator *self, Uint32 dt);
static BufferedGaugeOps alt_indicator_ops = {
    .render = (BufferRenderFunc)alt_indicator_render
};


AltIndicator *alt_indicator_new(void)
{
    AltIndicator *self;
    self = calloc(1, sizeof(AltIndicator));
    if(self){
        if(!alt_indicator_init(self)){
            free(self);
            return NULL;
        }
    }
    return self;
}

AltIndicator *alt_indicator_init(AltIndicator *self)
{
    PCF_Font *fnt;

    buffered_gauge_init(BUFFERED_GAUGE(self), &alt_indicator_ops, 68, 240+20);
    BUFFERED_GAUGE(self)->max_ops = 18;

    /*TODO: Change size to size - 20, when size becomes a parameter !
     * temporary fixed in the rendering function by drawing the ladder first
     * and then drawing on it
     * */
    self->ladder = ladder_gauge_new((LadderPageDescriptor *)alt_ladder_page_descriptor_new(), -1);
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->ladder),
        BUFFERED_GAUGE(self),
        0, 19
    );

    fnt = resource_manager_get_font(TERMINUS_18);
    DigitBarrel *db = digit_barrel_new(fnt, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(fnt, 0, 99, 10);
    self->odo = odo_gauge_new_multiple(-1, 4,
            -1, db2,
            -2, db,
            -2, db,
            -2, db
    );
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->odo),
        BUFFERED_GAUGE(self->ladder),
        (BASE_GAUGE(self->ladder)->w - BASE_GAUGE(self->odo)->w-1) -1,/*The last -1 in there to prevent eating the border*/
        19 + (BASE_GAUGE(self->ladder)->h-1)/2.0 - BASE_GAUGE(self->odo)->h/2.0 +1
    );

    buffered_gauge_clear(BUFFERED_GAUGE(self));

    self->talt_txt = text_gauge_new(NULL, true, 68, 20);
    self->talt_txt->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->talt_txt,
        resource_manager_get_static_font(TERMINUS_16,
            &SDL_WHITE,
            1, PCF_DIGITS
        )
    );
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->talt_txt),
        BUFFERED_GAUGE(self),
        0, 0
    );
    text_gauge_set_color(self->talt_txt, SDL_BLACK, BACKGROUND_COLOR);
    text_gauge_set_value(self->talt_txt, "0");

    self->qnh_txt = text_gauge_new(NULL, true, 68, 21);
    self->qnh_txt->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    text_gauge_set_static_font(self->qnh_txt,
        resource_manager_get_static_font(TERMINUS_16,
            &SDL_WHITE,
            1, PCF_DIGITS
        )
    );
    buffered_gauge_share_buffer(BUFFERED_GAUGE(self->qnh_txt),
        BUFFERED_GAUGE(self),
        0,
        BASE_GAUGE(self)->h - 20 -2
    );
    text_gauge_set_color(self->talt_txt, SDL_BLACK, BACKGROUND_COLOR);

#if USE_SDL_GPU
    self->gps_flag = SDL_CreateRGBSurfaceWithFormat(0, 68-2, 21, 24, SDL_PIXELFORMAT_RGB24);
#else
    self->gps_flag = SDL_CreateRGBSurface(0,
        68 - 2,
        21,
        buffered_gauge_get_view(BUFFERED_GAUGE(self))->format->BitsPerPixel,
        buffered_gauge_get_view(BUFFERED_GAUGE(self))->format->Rmask,
        buffered_gauge_get_view(BUFFERED_GAUGE(self))->format->Gmask,
        buffered_gauge_get_view(BUFFERED_GAUGE(self))->format->Bmask,
        buffered_gauge_get_view(BUFFERED_GAUGE(self))->format->Amask
    );
#endif
    if(!self->gps_flag) return NULL; //TODO: Free all above allocated resources+find a pattern for that case
    view_font_draw_text(self->gps_flag,
        NULL, HALIGN_CENTER | VALIGN_MIDDLE,
        "GPS",
        resource_manager_get_font(TERMINUS_16),
        SDL_URED(self->gps_flag), SDL_UBLACK(self->gps_flag)
    );
#if USE_SDL_GPU
    self->tgps_flag = GPU_CopyImageFromSurface(self->gps_flag);
#endif
    return self;
}

void alt_indicator_free(AltIndicator *self)
{
    ladder_gauge_free(self->ladder);
    odo_gauge_free(self->odo);
    text_gauge_free(self->talt_txt);
    text_gauge_free(self->qnh_txt);
    SDL_FreeSurface(self->gps_flag);
    buffered_gauge_dispose(BUFFERED_GAUGE(self));
    free(self);
}

bool alt_indicator_set_value(AltIndicator *self, float value)
{

    animated_gauge_set_value(ANIMATED_GAUGE(self->ladder), value);
    BUFFERED_GAUGE(self)->damaged = true;
    return odo_gauge_set_value(self->odo, value);
}

/*HPa*/
void alt_indicator_set_qnh(AltIndicator *self, float value)
{
    char number[5]; //4 digits, final 0x0
    int ivalue;

    ivalue = round(value);

    if(self->qnh != ivalue){
        self->qnh = ivalue;
        snprintf(number, 5, "%04d", self->qnh);
        text_gauge_set_value(self->qnh_txt, number);
    }
}

static void alt_indicator_render(AltIndicator *self, Uint32 dt)
{
    buffered_gauge_clear(BUFFERED_GAUGE(self));
    buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->talt_txt), dt);

    buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->ladder), dt);
    buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->odo), dt);

    if(self->src != ALT_SRC_GPS){
        buffered_gauge_paint_buffer(BUFFERED_GAUGE(self->qnh_txt), dt);
    }else{
        buffered_gauge_draw_outline(BUFFERED_GAUGE(self->qnh_txt), &SDL_WHITE, NULL);
#if USE_SDL_GPU
        buffered_gauge_blit_texture(BUFFERED_GAUGE(self), self->tgps_flag, NULL, &(SDL_Rect){
            .x = 0 + 1,
            .y = BASE_GAUGE(self)->h - 20 -2 + 1,
            .h = 21 - 1,
            .w = BASE_GAUGE(self)->w - 2
        });
#else
        buffered_gauge_blit(BUFFERED_GAUGE(self), self->gps_flag, NULL, &(SDL_Rect){
            .x = 0 + 1,
            .y = BASE_GAUGE(self)->h - 20 -2 + 1,
            .h = 21 - 1,
            .w = BASE_GAUGE(self)->w - 2
        });
#endif
    }

    buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, NULL);

    if(animated_gauge_moving(ANIMATED_GAUGE(self->ladder)) || animated_gauge_moving(ANIMATED_GAUGE(self->odo)))
        BUFFERED_GAUGE(self)->damaged = true;
}
