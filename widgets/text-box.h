/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef TEXT_BOX_H
#define TEXT_BOX_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL_pcf.h"
#include "base-widget.h"
#include "resource-manager.h"

typedef struct _TextBox TextBox;

typedef void (*TextBoxTextChanged)(TextBox *self, void *userdata);

typedef struct{
    PCF_StaticFontRectPatch *patches;
    size_t npatches;
    size_t apatches;
    size_t first_index;

    Uint32 startx; /*offset in the virtual rectangle*/
}TextBoxState;

typedef struct _TextBox{
    BaseWidget super;

    FontResource font_id;
    PCF_StaticFont *sfont;

    /*TODO: GenericArray/StringBuilder*/
    char *text;
    size_t alen;
    size_t tlen; /*text len, INCLUDING last ''\0' byte*/

    size_t current_index;

    char *allowed_chars;
    size_t nallowed_chars;

    SDL_Rect text_size; /*Virtual rectangle with the whole text*/

    TextBoxState state;

    TextBoxTextChanged changed_callback;
    void *userdata;
}TextBox;

TextBox *text_box_new(FontResource font_id, int width, int height);
TextBox *text_box_init(TextBox *self, FontResource font_id, int width, int height);

bool text_box_set_text(TextBox *self, const char *text);
bool text_box_set_allowed_chars(TextBox *self, bool keep_order, int nsets, ...);

bool text_box_move_cursor(TextBox *self, int_fast8_t direction);
void text_box_cycle_char(TextBox *self, int_fast8_t direction);

#endif /* TEXT_BOX_H */
