#include <stdio.h>
#include <stdlib.h>

#include <SDL_gpu.h>

#include "animated-gauge.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "side-panel.h"

#include "elevator-gauge.h"
#include "fishbone-gauge.h"
#include "misc.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "text-gauge.h"

#define GROUP_SPACE 8

static void side_panel_render(SidePanel *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location);
static BaseGaugeOps side_panel_ops = {
    .render = (RenderFunc)side_panel_render
};

SidePanel *side_panel_new(int width, int height)
{
    SidePanel *rv;

    rv = calloc(1, sizeof(SidePanel));
    if(rv){
        if(!side_panel_init(rv, width, height)){
            free(rv);
            return NULL;
        }
    }
    return rv;
}

/**
 *
 * Params are there for future use. As for now, only supported dimensions
 * are 92x480.
 *
 */
SidePanel *side_panel_init(SidePanel *self, int width, int height)
{

    width = 95;
    height = 480;

    base_gauge_init(
        BASE_GAUGE(self),
        &side_panel_ops,
        width,
        height
    );
#if 0
    self->egt = elevator_gauge_new(true,
        Left,
        resource_manager_get_font(TERMINUS_12), SDL_WHITE,
        300, 2300, 300,
        15, 80,
        0, NULL
    );
    self->egt_txt = text_gauge_new("EGT °F", false, 92 - 10, 12);
    text_gauge_set_static_font(self->egt_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->locations[EGT] = (SDL_Rect){
        .x = 0,
        .y = 0,
        .w = BASE_GAUGE(self->egt)->w,
        .h = BASE_GAUGE(self->egt)->h
    };
    self->locations[EGT_TXT] = (SDL_Rect){
        .x = 0,
        .y = self->locations[EGT].y + self->locations[EGT].h + 1,
        .w =  BASE_GAUGE(self->egt_txt)->w,
        .h =  BASE_GAUGE(self->egt_txt)->h,
    };
#else
     self->locations[EGT] = self->locations[EGT_TXT] = (SDL_Rect){0,0,0,0};
#endif
    self->rpm = elevator_gauge_new(true,
        Left,
        resource_manager_get_font(TERMINUS_12), SDL_WHITE,
        0, 3000, 300,
        15, 110,
        3,(ColorZone[]){{
            .from = 0,
            .to = 2600,
            .color = SDL_GREEN,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 2600,
        .to = 2800,
        .color = SDL_YELLOW,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 2800,
        .to = 3000,
        .color = SDL_RED,
        .flags = FromExcluded | ToIncluded
        }}
    );
    self->rpm_txt = text_gauge_new("RPM", false, 92 - 10, 12);
    text_gauge_set_static_font(self->rpm_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->locations[RPM] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[EGT_TXT]) + 4,
        .w = BASE_GAUGE(self->rpm)->w,
        .h = BASE_GAUGE(self->rpm)->h
    };
    self->locations[RPM_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[RPM]) + 2,
        .w = BASE_GAUGE(self->rpm_txt)->w,
        .h = BASE_GAUGE(self->rpm_txt)->h

    };

    /*Fuel flow*/
    self->fuel_flow_txt = text_gauge_new("FUEL FLOW", false, 92 - 10, 12);
    text_gauge_set_static_font(self->fuel_flow_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->fuel_flow_value = text_gauge_new("GPH", false, 92 - 10, 12);
    text_gauge_set_static_font(self->fuel_flow_value,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->locations[FUEL_FLOW_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[RPM_TXT]) + GROUP_SPACE,
        .w = BASE_GAUGE(self->fuel_flow_txt)->w,
        .h = BASE_GAUGE(self->fuel_flow_txt)->h
    };
    self->locations[FUEL_FLOW_VALUE] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_FLOW_TXT]) + 2,
        .w = BASE_GAUGE(self->fuel_flow_value)->w,
        .h = BASE_GAUGE(self->fuel_flow_value)->h
    };

    /*Oil temperature*/
    self->oil_temp_txt  = text_gauge_new("OIL TEMP", false, 92 - 10, 12);
    text_gauge_set_static_font(self->oil_temp_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->oil_temp = fishbone_gauge_new(true,
        resource_manager_get_font(TERMINUS_12), SDL_WHITE,
        0, 300, -1,
        92 - 10, 15,
        3,(ColorZone[]){{
            .from = 0,
            .to = 200,
            .color = SDL_GREEN,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 200,
        .to = 250,
        .color = SDL_YELLOW,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 250,
        .to = 300,
        .color = SDL_RED,
        .flags = FromExcluded | ToIncluded
        }}
    );
    self->locations[OIL_TEMP_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_FLOW_VALUE]) + GROUP_SPACE,
        .w = BASE_GAUGE(self->oil_temp_txt)->w,
        .h = BASE_GAUGE(self->oil_temp_txt)->h
    };
    self->locations[OIL_TEMP] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[OIL_TEMP_TXT]) +2,
        .w = BASE_GAUGE(self->oil_temp)->w,
        .h = BASE_GAUGE(self->oil_temp)->h
    };

    /*Oil pressure*/
    self->oil_press_txt  = text_gauge_new("OIL PRESS", false, 92 - 10, 12);
    text_gauge_set_static_font(self->oil_press_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->oil_press = fishbone_gauge_new(true,
        resource_manager_get_font(TERMINUS_12), SDL_WHITE,
        0, 100, 20,
        92 - 10, 15,
        4,(ColorZone[]){{
            .from = 0,
            .to = 20,
            .color = SDL_RED,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 20,
        .to = 60,
        .color = SDL_GREEN,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 60,
        .to = 80,
        .color = SDL_YELLOW,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 80,
        .to = 100,
        .color = SDL_RED,
        .flags = FromExcluded | ToIncluded
        }}
    );
    self->locations[OIL_PRESS_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[OIL_TEMP]) + GROUP_SPACE,
        .w = BASE_GAUGE(self->oil_press_txt)->w,
        .h = BASE_GAUGE(self->oil_press_txt)->h
    };
    self->locations[OIL_PRESS] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[OIL_PRESS_TXT]) +2,
        .w = BASE_GAUGE(self->oil_press)->w,
        .h = BASE_GAUGE(self->oil_press)->h
    };

    /*CHT*/
    self->cht_txt  = text_gauge_new("CHT", false, 92 - 10, 12);
    text_gauge_set_static_font(self->cht_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->cht = fishbone_gauge_new(true,
        resource_manager_get_font(TERMINUS_12), SDL_WHITE,
        0, 600, -1,
        92 - 10, 15,
        3,(ColorZone[]){{
            .from = 0,
            .to = 400,
            .color = SDL_GREEN,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 400,
        .to = 550,
        .color = SDL_YELLOW,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 550,
        .to = 600,
        .color = SDL_RED,
        .flags = FromExcluded | ToIncluded
        }}
    );
    self->locations[CHT_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[OIL_PRESS]) + GROUP_SPACE,
        .w = BASE_GAUGE(self->cht_txt)->w,
        .h = BASE_GAUGE(self->cht_txt)->h
    };
    self->locations[CHT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[CHT_TXT]) +2,
        .w = BASE_GAUGE(self->cht)->w,
        .h = BASE_GAUGE(self->cht)->h
    };

    /*Fuel pressure*/
    self->fuel_px_txt  = text_gauge_new("FUEL PRESSURE", false, 92 - 10, 12);
    text_gauge_set_static_font(self->fuel_px_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->fuel_px = fishbone_gauge_new(true,
        resource_manager_get_font(TERMINUS_12), SDL_WHITE,
        0, 9, -1,
        92 - 10, 15,
        3,(ColorZone[]){{
            .from = 0,
            .to = 2,
            .color = SDL_RED,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 2,
        .to = 6,
        .color = SDL_GREEN,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 6,
        .to = 9,
        .color = SDL_RED,
        .flags = FromExcluded | ToIncluded
        }}
    );
    self->locations[FUEL_PX_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[CHT]) + GROUP_SPACE,
        .w = BASE_GAUGE(self->fuel_px_txt)->w,
        .h = BASE_GAUGE(self->fuel_px_txt)->h
    };
    self->locations[FUEL_PX] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_PX_TXT]) +2,
        .w = BASE_GAUGE(self->fuel_px)->w,
        .h = BASE_GAUGE(self->fuel_px)->h
    };

    /*Fuel QTY*/
    self->fuel_qty_txt  = text_gauge_new("FUEL QTY GAL", false, 92 - 10, 12);
    text_gauge_set_static_font(self->fuel_qty_txt,
        resource_manager_get_static_font(TERMINUS_12,
            &SDL_WHITE,
            2, PCF_ALPHA, PCF_DIGITS
        )
    );
    self->fuel_qty = fishbone_gauge_new(true,
        resource_manager_get_font(TERMINUS_12), SDL_WHITE,
        0, 25, 5,
        92 - 10, 15,
        3,(ColorZone[]){{
            .from = 0,
            .to = 2,
            .color = SDL_RED,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 2,
        .to = 10,
        .color = SDL_YELLOW,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 10,
        .to = 25,
        .color = SDL_GREEN,
        .flags = FromExcluded | ToIncluded
        }}
    );
    self->locations[FUEL_QTY_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_PX]) + GROUP_SPACE,
        .w = BASE_GAUGE(self->fuel_qty_txt)->w,
        .h = BASE_GAUGE(self->fuel_qty_txt)->h
    };
    self->locations[FUEL_QTY] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_QTY_TXT]) +2,
        .w = BASE_GAUGE(self->fuel_qty)->w,
        .h = BASE_GAUGE(self->fuel_qty)->h

    };
#if 1
    SDL_Rect reference = {0,0,BASE_GAUGE(self)->w,BASE_GAUGE(self)->h};
    for(int i = 0; i < NSidePanelLocations; i++){
        int tmpy = self->locations[i].y;
        SDLExt_RectAlign(&self->locations[i], &reference, HALIGN_CENTER);
        self->locations[i].y = tmpy;

    }
#endif
    return self;
}

void side_panel_set_rpm(SidePanel *self, float value)
{
    char buffer[10];

    animated_gauge_set_value(ANIMATED_GAUGE(self->rpm), value);

    snprintf(buffer, 10, "%04d RPM", (int)value);
    text_gauge_set_value(self->rpm_txt, buffer);
}

void side_panel_set_fuel_flow(SidePanel *self, float value)
{
    char buffer[10];

    snprintf(buffer, 10, "%0.2f GPH", value);
    text_gauge_set_value(self->fuel_flow_value, buffer);
}

void side_panel_set_oil_temp(SidePanel *self, float value)
{

    animated_gauge_set_value(ANIMATED_GAUGE(self->oil_temp), value);
}

void side_panel_set_oil_press(SidePanel *self, float value)
{

    animated_gauge_set_value(ANIMATED_GAUGE(self->oil_press), value);
}

void side_panel_set_cht(SidePanel *self, float value)
{
    animated_gauge_set_value(ANIMATED_GAUGE(self->cht), value);
}

void side_panel_set_fuel_px(SidePanel *self, float value)
{
    animated_gauge_set_value(ANIMATED_GAUGE(self->fuel_px), value);
}

void side_panel_set_fuel_qty(SidePanel *self, float value)
{
    animated_gauge_set_value(ANIMATED_GAUGE(self->fuel_qty), value);
}

#define rectf_offset(r1, r2) ((GPU_Rect){(r1)->x + (r2)->x, (r1)->y + (r2)->y, (r1)->w, (r1)->h})
#define rect_offset(r1, r2) ((SDL_Rect){(r1)->x + (r2)->x, (r1)->y + (r2)->y, (r1)->w, (r1)->h})
static void side_panel_render(SidePanel *self, Uint32 dt, SDL_Surface *destination, SDL_Rect *location)
{
    SDL_Rect area = (SDL_Rect){0,0, BASE_GAUGE(self)->w, BASE_GAUGE(self)->h};

    GPU_RectangleFilled2(gpu_screen, rectf_offset(&area, location), SDL_BLACK);
//    buffered_gauge_clear(BUFFERED_GAUGE(self));
//    buffered_gauge_draw_outline(BUFFERED_GAUGE(self), &SDL_WHITE, NULL); /*Not really needed*/
#if 0
    base_gauge_render(BASE_GAUGE(self->egt), dt, destination, &self->locations[EGT]);
    base_gauge_render(BASE_GAUGE(self->egt_txt), dt, destination, &self->locations[EGT_TXT]);
#endif
    base_gauge_render(BASE_GAUGE(self->rpm), dt, destination, &rect_offset(&self->locations[RPM], location));
    base_gauge_render(BASE_GAUGE(self->rpm_txt), dt, destination, &rect_offset(&self->locations[RPM_TXT], location));

    base_gauge_render(BASE_GAUGE(self->fuel_flow_txt), dt, destination, &rect_offset(&self->locations[FUEL_FLOW_TXT], location));
    base_gauge_render(BASE_GAUGE(self->fuel_flow_value), dt, destination, &rect_offset(&self->locations[FUEL_FLOW_VALUE], location));

    base_gauge_render(BASE_GAUGE(self->oil_temp), dt, destination, &rect_offset(&self->locations[OIL_TEMP], location));
    base_gauge_render(BASE_GAUGE(self->oil_temp_txt), dt, destination, &rect_offset(&self->locations[OIL_TEMP_TXT], location));

    base_gauge_render(BASE_GAUGE(self->oil_press), dt, destination, &rect_offset(&self->locations[OIL_PRESS], location));
    base_gauge_render(BASE_GAUGE(self->oil_press_txt), dt, destination, &rect_offset(&self->locations[OIL_PRESS_TXT], location));

    base_gauge_render(BASE_GAUGE(self->cht), dt, destination, &rect_offset(&self->locations[CHT], location));
    base_gauge_render(BASE_GAUGE(self->cht_txt), dt, destination, &rect_offset(&self->locations[CHT_TXT], location));

    base_gauge_render(BASE_GAUGE(self->fuel_px), dt, destination, &rect_offset(&self->locations[FUEL_PX], location));
    base_gauge_render(BASE_GAUGE(self->fuel_px_txt), dt, destination, &rect_offset(&self->locations[FUEL_PX_TXT], location));

    base_gauge_render(BASE_GAUGE(self->fuel_qty), dt, destination, &rect_offset(&self->locations[FUEL_QTY], location));
    base_gauge_render(BASE_GAUGE(self->fuel_qty_txt), dt, destination, &rect_offset(&self->locations[FUEL_QTY_TXT], location));
}
