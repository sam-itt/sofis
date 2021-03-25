/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_pixels.h"
#include "SDL_pcf.h"

#include "resource-manager.h"
#include "misc.h"
#include <res-dirs.h>

static ResourceManager *_instance = NULL;
static void resource_manager_push_static_font(PCF_StaticFont *font, FontResource creator);

static ResourceManager *resource_manager_new(void)
{
    ResourceManager *self;

    self = calloc(1, sizeof(ResourceManager));
    if(self){
        /*init as needed*/
    }
    return self;
}

static inline ResourceManager *resource_manager_get_instance(void)
{
    if(!_instance)
        _instance = resource_manager_new();
    return _instance;
}

void resource_manager_shutdown(void)
{
    ResourceManager *self;

    self = _instance;
    if(!self) return;

    for(int i = 0; i < FONT_MAX; i++){
        if(self->fonts[i]){
            if(self->fonts[i]->xfont.refcnt > 1){
                printf(
                    "ResourceManager: Font %d refcount was still %d at shutdown (1 expected), leaking %p\n",
                    i,
                    self->fonts[i]->xfont.refcnt,
                    self->fonts[i]
                );
            }
            self->fonts[i]->xfont.refcnt--;
            PCF_CloseFont(self->fonts[i]);
        }
    }

    for(int i = 0; i < self->n_sfonts; i++){
        if(self->sfonts[i].font->refcnt > 1){
            printf(
                "ResourceManager: StaticFont %d refcount was still %d at shutdown (1 expected), leaking %p\n",
                i,
                self->sfonts[i].font->refcnt,
                self->sfonts[i].font
            );
        }
        self->sfonts[i].font->refcnt--;
        PCF_FreeStaticFont(self->sfonts[i].font);
    }
    if(self->sfonts)
        free(self->sfonts);
    free(self);
    _instance = NULL;
}

static const char *resource_manager_get_font_filename(FontResource font)
{
    switch (font) {
        case TERMINUS_12:
            return FONT_DIR"/ter-x12n.pcf.gz";
        case TERMINUS_14:
            return FONT_DIR"/ter-x14n.pcf.gz";
        case TERMINUS_16:
            return FONT_DIR"/ter-x16n.pcf.gz";
        case TERMINUS_18:
            return FONT_DIR"/ter-x18n.pcf.gz";
        case TERMINUS_24:
            return FONT_DIR"/ter-x24n.pcf.gz";
        case TERMINUS_32:
            return FONT_DIR"/ter-x32n.pcf.gz";
        case FONT_MAX:
        default:
            return NULL;
    }
}

PCF_Font *resource_manager_get_font(FontResource font)
{
    ResourceManager *self;

    self = resource_manager_get_instance();
    if(!self->fonts[font]){
        self->fonts[font] = PCF_OpenFont(resource_manager_get_font_filename(font));
        if(self->fonts[font])
            self->fonts[font]->xfont.refcnt++;
    }
    return self->fonts[font];
}

PCF_StaticFont *resource_manager_get_static_font(FontResource font, SDL_Color *color, int nsets, ...)
{
    char *str;
    size_t tlen;
    va_list ap;
    ResourceManager *self;
    PCF_StaticFont *rv;

    self = resource_manager_get_instance();


    tlen = 0;
    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        tlen += strlen(va_arg(ap, char*));
    }
    va_end(ap);

    str = malloc((tlen + 1) * sizeof(char));
    if(!str) return NULL;

    char *iter = str;
    va_start(ap, nsets);
    for(int i = 0; i < nsets; i++){
        char *tmp = va_arg(ap, char*);
        strcpy(iter, tmp);
        iter += strlen(tmp)*sizeof(char);
    }
    va_end(ap);

    qsort(str, strlen(str), sizeof(char), (__compar_fn_t) strcmp);
    filter_dedup(str, tlen);
//    printf("ResourceManager: looking for a (r:%d,g:%d,b:%d,a:%d) font of type %d with the following charset: %s\n",color->r,color->g,color->b,color->a, font, str);
    for(int i = 0; i < self->n_sfonts; i++){
        if(self->sfonts[i].creator != font) continue;
        if(PCF_StaticFontCanWrite(self->sfonts[i].font, color, str)){
//            printf("Found existing instance %p(glyphs: %s)\n", self->sfonts[i].font, self->sfonts[i].font->glyphs);
            rv = self->sfonts[i].font;
            goto end;
        }
    }

    va_start(ap, nsets);
    rv = PCF_FontCreateStaticFontVA(
        resource_manager_get_font(font),
        color,
        nsets,
        tlen, ap
    );
    va_end(ap);
    resource_manager_push_static_font(rv, font);
//    printf("Not found, created a new one: %p(glyphs: %s)\n", rv, rv->glyphs);

end:
    free(str);
    return rv;
}

static void resource_manager_push_static_font(PCF_StaticFont *font, FontResource creator)
{
    ResourceManager *self;

    self = resource_manager_get_instance();
    if(self->n_sfonts == self->n_allocated){
        StaticFontResource *tmp;
        tmp = realloc(self->sfonts, (self->n_allocated + 4) * sizeof(StaticFontResource));
        if(!tmp)
            return; //TODO: OOM / Check leaks
        self->sfonts = tmp;
        self->n_allocated += 4;
    }
    self->sfonts[self->n_sfonts].font = font;
    self->sfonts[self->n_sfonts].creator = creator;
    self->n_sfonts++;
    font->refcnt++;
}
