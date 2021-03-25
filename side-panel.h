/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef SIDE_PANEL_H
#define SIDE_PANEL_H

#include "base-gauge.h"
#include "elevator-gauge.h"
#include "fishbone-gauge.h"
#include "text-gauge.h"

typedef enum{
    EGT,
    EGT_TXT,

    RPM,
    RPM_TXT,

    FUEL_FLOW_TXT,
    FUEL_FLOW_VALUE,

    OIL_TEMP_TXT,
    OIL_TEMP,

    OIL_PRESS_TXT,
    OIL_PRESS,

    CHT_TXT,
    CHT,

    FUEL_PX_TXT,
    FUEL_PX,

    FUEL_QTY_TXT,
    FUEL_QTY,

    NSidePanelLocations
}SidePanelLocations;

typedef struct{
    BaseGauge super;

    ElevatorGauge *egt;
    TextGauge *egt_txt;

    ElevatorGauge *rpm;
    TextGauge *rpm_txt;

    TextGauge *fuel_flow_txt;
    TextGauge *fuel_flow_value;

    TextGauge *oil_temp_txt;
    FishboneGauge *oil_temp;

    TextGauge *oil_press_txt;
    FishboneGauge *oil_press;

    TextGauge *cht_txt;
    FishboneGauge *cht;

    TextGauge *fuel_px_txt;
    FishboneGauge *fuel_px;

    TextGauge *fuel_qty_txt;
    FishboneGauge *fuel_qty;

    SDL_Rect locations[NSidePanelLocations];
}SidePanel;

SidePanel *side_panel_new(int width, int height);
SidePanel *side_panel_init(SidePanel *self, int width, int height);

void side_panel_set_rpm(SidePanel *self, float value);
void side_panel_set_fuel_flow(SidePanel *self, float value);
void side_panel_set_oil_temp(SidePanel *self, float value);
void side_panel_set_oil_press(SidePanel *self, float value);
void side_panel_set_cht(SidePanel *self, float value);
void side_panel_set_fuel_px(SidePanel *self, float value);
void side_panel_set_fuel_qty(SidePanel *self, float value);

#endif /* SIDE_PANEL_H */

