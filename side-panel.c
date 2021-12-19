/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>

#include "base-gauge.h"
#include "elevator-gauge.h"
#include "fishbone-gauge.h"
#include "side-panel.h"

#include "misc.h"
#include "resource-manager.h"
#include "sdl-colors.h"

#define GROUP_SPACE 8

static void side_panel_render(SidePanel *self, Uint32 dt, RenderContext *ctx);
static void side_panel_update_state(SidePanel *self, Uint32 dt);
static BaseGaugeOps side_panel_ops = {
    .render = (RenderFunc)side_panel_render,
    .update_state = (StateUpdateFunc)side_panel_update_state,
    .dispose = (DisposeFunc)NULL
};

SidePanel *side_panel_new(int width, int height)
{
    SidePanel *rv;

    rv = calloc(1, sizeof(SidePanel));
    if(rv){
        if(!side_panel_init(rv, width, height)){
            return base_gauge_free(BASE_GAUGE(rv));
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
    height = 480-1; /*SDL_gpu has a strange coordinates handling*/

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
        .w = base_gauge_w(BASE_GAUGE(self->rpm)),
        .h = base_gauge_h(BASE_GAUGE(self->rpm))
    };
    self->locations[RPM_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[RPM]) + 2,
        .w = base_gauge_w(BASE_GAUGE(self->rpm_txt)),
        .h = base_gauge_h(BASE_GAUGE(self->rpm_txt))
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
            3, PCF_ALPHA, PCF_DIGITS, "."
        )
    );
    self->locations[FUEL_FLOW_TXT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[RPM_TXT]) + GROUP_SPACE,
        .w = base_gauge_w(BASE_GAUGE(self->fuel_flow_txt)),
        .h = base_gauge_h(BASE_GAUGE(self->fuel_flow_txt))
    };
    self->locations[FUEL_FLOW_VALUE] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_FLOW_TXT]) + 2,
        .w = base_gauge_w(BASE_GAUGE(self->fuel_flow_value)),
        .h = base_gauge_h(BASE_GAUGE(self->fuel_flow_value))
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
        .w = base_gauge_w(BASE_GAUGE(self->oil_temp_txt)),
        .h = base_gauge_h(BASE_GAUGE(self->oil_temp_txt))
    };
    self->locations[OIL_TEMP] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[OIL_TEMP_TXT]) +2,
        .w = base_gauge_w(BASE_GAUGE(self->oil_temp)),
        .h = base_gauge_h(BASE_GAUGE(self->oil_temp))
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
        .w = base_gauge_w(BASE_GAUGE(self->oil_press_txt)),
        .h = base_gauge_h(BASE_GAUGE(self->oil_press_txt))
    };
    self->locations[OIL_PRESS] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[OIL_PRESS_TXT]) +2,
        .w = base_gauge_w(BASE_GAUGE(self->oil_press)),
        .h = base_gauge_h(BASE_GAUGE(self->oil_press))
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
        .w = base_gauge_w(BASE_GAUGE(self->cht_txt)),
        .h = base_gauge_h(BASE_GAUGE(self->cht_txt))
    };
    self->locations[CHT] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[CHT_TXT]) +2,
        .w = base_gauge_w(BASE_GAUGE(self->cht)),
        .h = base_gauge_h(BASE_GAUGE(self->cht))
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
        .w = base_gauge_w(BASE_GAUGE(self->fuel_px_txt)),
        .h = base_gauge_h(BASE_GAUGE(self->fuel_px_txt))
    };
    self->locations[FUEL_PX] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_PX_TXT]) +2,
        .w = base_gauge_w(BASE_GAUGE(self->fuel_px)),
        .h = base_gauge_h(BASE_GAUGE(self->fuel_px))
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
        .w = base_gauge_w(BASE_GAUGE(self->fuel_qty_txt)),
        .h = base_gauge_h(BASE_GAUGE(self->fuel_qty_txt))
    };
    self->locations[FUEL_QTY] = (SDL_Rect){
        .x = 0,
        .y = SDLExt_RectLastY(&self->locations[FUEL_QTY_TXT]) +2,
        .w = base_gauge_w(BASE_GAUGE(self->fuel_qty)),
        .h = base_gauge_h(BASE_GAUGE(self->fuel_qty))

    };

#if 1
    SDL_Rect reference = {0,0,base_gauge_w(BASE_GAUGE(self)),base_gauge_h(BASE_GAUGE(self))};
    for(int i = 0; i < NSidePanelLocations; i++){
        int tmpy = self->locations[i].y;
        SDLExt_RectAlign(&self->locations[i], &reference, HALIGN_CENTER);
        self->locations[i].y = tmpy;

    }
#endif
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->rpm_txt),
        self->locations[RPM_TXT].x,
        self->locations[RPM_TXT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->rpm),
        self->locations[RPM].x,
        self->locations[RPM].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->fuel_flow_txt),
        self->locations[FUEL_FLOW_TXT].x,
        self->locations[FUEL_FLOW_TXT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->fuel_flow_value),
        self->locations[FUEL_FLOW_VALUE].x,
        self->locations[FUEL_FLOW_VALUE].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->oil_temp_txt),
        self->locations[OIL_TEMP_TXT].x,
        self->locations[OIL_TEMP_TXT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->oil_temp),
        self->locations[OIL_TEMP].x,
        self->locations[OIL_TEMP].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->oil_press_txt),
        self->locations[OIL_PRESS_TXT].x,
        self->locations[OIL_PRESS_TXT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->oil_press),
        self->locations[OIL_PRESS].x,
        self->locations[OIL_PRESS].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->cht_txt),
        self->locations[CHT_TXT].x,
        self->locations[CHT_TXT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->cht),
        self->locations[CHT].x,
        self->locations[CHT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->fuel_px_txt),
        self->locations[FUEL_PX_TXT].x,
        self->locations[FUEL_PX_TXT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->fuel_px),
        self->locations[FUEL_PX].x,
        self->locations[FUEL_PX].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->fuel_qty_txt),
        self->locations[FUEL_QTY_TXT].x,
        self->locations[FUEL_QTY_TXT].y
    );
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->fuel_qty),
        self->locations[FUEL_QTY].x,
        self->locations[FUEL_QTY].y
    );
    return self;
}

void side_panel_set_rpm(SidePanel *self, float value)
{
    elevator_gauge_set_value(self->rpm, value, true);
    text_gauge_set_value_formatn(self->rpm_txt,
        10,
        "%04d RPM", (int)value
    );
}

void side_panel_set_fuel_flow(SidePanel *self, float value)
{
    text_gauge_set_value_formatn(self->fuel_flow_value,
        10,
        "%0.2f GPH", value
    );
}

void side_panel_set_oil_temp(SidePanel *self, float value)
{
    fishbone_gauge_set_value(self->oil_temp, value, true);
}

void side_panel_set_oil_press(SidePanel *self, float value)
{
    fishbone_gauge_set_value(self->oil_press, value, true);
}

void side_panel_set_cht(SidePanel *self, float value)
{
    fishbone_gauge_set_value(self->cht, value, true);
}

void side_panel_set_fuel_px(SidePanel *self, float value)
{
    fishbone_gauge_set_value(self->fuel_px, value, true);
}

void side_panel_set_fuel_qty(SidePanel *self, float value)
{
    fishbone_gauge_set_value(self->fuel_qty, value, true);
}

static void side_panel_update_state(SidePanel *self, Uint32 dt)
{


}

static void side_panel_render(SidePanel *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_fill(BASE_GAUGE(self),ctx, NULL, &SDL_BLACK, false);
    base_gauge_draw_outline(BASE_GAUGE(self), ctx, &SDL_WHITE, NULL);
}
