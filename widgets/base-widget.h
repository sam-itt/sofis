/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef BASE_WIDGET_H
#define BASE_WIDGET_H
#include <stdbool.h>

#include "base-gauge.h"

#include "sdl-colors.h"

typedef bool (*EventHandlerFunc)(void *self, SDL_KeyboardEvent *event);

typedef struct{
    BaseGaugeOps super;
    EventHandlerFunc handle_event; /*events from SDL (keyboard/mouse)*/
}BaseWidgetOps;

typedef struct _BaseWidget{
    BaseGauge super;

    bool has_focus;
}BaseWidget;

/*events emitted by the widget: selection changed, etc.*/
typedef void (*EventListenerFunc)(void *self, BaseWidget *sender);
typedef struct{
    EventListenerFunc callback;
    void *target;
}EventListener;

#define BASE_WIDGET(self) ((BaseWidget*)(self))
#define BASE_WIDGET_OPS(self) ((BaseWidgetOps*)(self))

static inline BaseWidget *base_widget_init(BaseWidget *self, BaseWidgetOps *ops,
                             int w, int h)
{

    return (BaseWidget *)base_gauge_init(BASE_GAUGE(self),
        BASE_GAUGE_OPS(ops),
        w, h
    );
}

/*return false when the widget wants to give up focus*/
static inline bool base_widget_handle_event(BaseWidget *self,
                                           SDL_KeyboardEvent *event)
{
    if(BASE_WIDGET_OPS(BASE_GAUGE(self)->ops)->handle_event)
        return BASE_WIDGET_OPS(BASE_GAUGE(self)->ops)->handle_event(self, event);
    return false;
}

static inline void base_widget_draw_outline(BaseWidget *self, RenderContext *ctx)
{
    base_gauge_draw_outline(BASE_GAUGE(self),
        ctx,
        self->has_focus ? &SDL_GREEN : &SDL_WHITE,
        NULL
    );
}
#endif /* BASE_WIDGET_H */
