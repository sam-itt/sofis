/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef DIRECT_TO_H
#define DIRECT_TO_H
#include "base-widget.h"
#include "button-flat.h"
#include "list-box.h"
#include "text-box.h"
#include "text-gauge.h"

typedef struct{
    BaseWidget super;
    bool visible;

    TextBox *text;
    ListBox *list;

    TextGauge *bearing_lbl;
    TextGauge *distance_lbl;

    TextGauge *bearing_value;
    TextGauge *distance_value;


    TextGauge *latitude;
    TextGauge *longitude;

    ButtonFlat *validate_button;

    BaseWidget *focused;
}DirectToDialog;


DirectToDialog *direct_to_dialog_new();
DirectToDialog *direct_to_dialog_init(DirectToDialog *self);
void direct_to_dialog_reset(DirectToDialog *self);
#endif /* DIRECT_TO_H */
