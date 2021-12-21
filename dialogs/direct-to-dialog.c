#include "airport.h"
#include "button.h"
#include "resource-manager.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "direct-to-dialog.h"

#include "base-gauge.h"
#include "base-widget.h"
#include "dialogs/airports-list-model.h"
#include "text-box.h"
#include "text-gauge.h"
#include "misc.h"
#include "data-source.h"
#include "geo-location.h"

static void direct_to_dialog_render(DirectToDialog *self, Uint32 dt, RenderContext *ctx);
static bool direct_to_dialog_handle_event(DirectToDialog *self, SDL_KeyboardEvent *event);
static void update_list_content(TextBox *txtbx, DirectToDialog *self);
static void selection_changed(DirectToDialog *self, ListBox *sender);
static void button_pressed(DirectToDialog *self, Button *sender);

static BaseWidgetOps direct_to_dialog_ops = {
   .super.render = (RenderFunc)direct_to_dialog_render,
   .super.update_state = (StateUpdateFunc)NULL,
   .super.dispose = (DisposeFunc)NULL,
   .handle_event = (EventHandlerFunc)direct_to_dialog_handle_event
};


DirectToDialog *direct_to_dialog_new()
{
    DirectToDialog *self;
    self = calloc(1, sizeof(DirectToDialog));
    if(self){
        if(!direct_to_dialog_init(self))
            return base_gauge_free(BASE_GAUGE(self));

    }
    return self;
}

DirectToDialog *direct_to_dialog_init(DirectToDialog *self)
{
    base_widget_init(BASE_WIDGET(self),
        &direct_to_dialog_ops,
        12*20, 304
    );

    PCF_Font *fnt = resource_manager_get_font(TERMINUS_24);
    self->text = text_box_new(
        TERMINUS_24,
        12*20,
        PCF_FontCharHeight(fnt)
    );
    text_box_set_allowed_chars(self->text, true, 3, " -", PCF_UPPER_CASE, PCF_DIGITS);
    self->text->changed_callback = (TextBoxTextChanged)update_list_content;
    self->text->userdata = self;

    self->list = list_box_new(
        TERMINUS_24,
        BASE_GAUGE(self->text)->frame.w,
        200
    );

    list_box_set_selection_changed_listener(self->list,
        (EventListenerFunc)selection_changed,
        self
    );

    self->focused = BASE_WIDGET(self->text);
    self->focused->has_focus = true;

    self->bearing_lbl = text_gauge_new("BRG", true, 35, 24);
    text_gauge_set_static_font(self->bearing_lbl,
        resource_manager_get_static_font(TERMINUS_24,
            &SDL_WHITE,
            1, PCF_ALPHA
        )
    );
    self->bearing_value = text_gauge_new(NULL, true, 52, 24);
    text_gauge_set_size(self->bearing_lbl, 4);
    self->bearing_value->alignment = VALIGN_BOTTOM | HALIGN_LEFT;
    text_gauge_set_static_font(self->bearing_value,
        resource_manager_get_static_font(TERMINUS_24,
            &SDL_CYAN,
            1, PCF_DIGITS
        )
    );


    self->distance_lbl = text_gauge_new("DIS", true, 35, 24);
    text_gauge_set_static_font(self->distance_lbl,
        resource_manager_get_static_font(TERMINUS_24,
            &SDL_WHITE,
            1, PCF_ALPHA
        )
    );
    self->distance_value = text_gauge_new(NULL, true, 60, 24);
    text_gauge_set_size(self->distance_value, 4);
    self->distance_value->alignment = VALIGN_BOTTOM | HALIGN_LEFT;
    text_gauge_set_static_font(self->distance_value,
        resource_manager_get_static_font(TERMINUS_24,
            &SDL_CYAN,
            1, PCF_DIGITS
        )
    );


    self->latitude = text_gauge_new(NULL, true, 135, 24);
    text_gauge_set_size(self->latitude, 14);
    self->latitude->alignment = VALIGN_BOTTOM | HALIGN_RIGHT;
    text_gauge_set_static_font(self->latitude,
        resource_manager_get_static_font(TERMINUS_24,
            &SDL_CYAN,
            2, PCF_DIGITS,
            "NSEW"
        )
    );

    self->longitude = text_gauge_new(NULL, true, 135, 24);
    text_gauge_set_size(self->longitude, 14);
    self->longitude->alignment = VALIGN_BOTTOM | HALIGN_RIGHT;
    text_gauge_set_static_font(self->longitude,
        resource_manager_get_static_font(TERMINUS_24,
            &SDL_CYAN,
            2, PCF_DIGITS,
            "NSEW"
        )
    );

    self->validate_button = button_new("Validate", TERMINUS_24,
        BASE_GAUGE(self)->frame.w, 24
    );
    self->validate_button->alignment = HALIGN_CENTER | VALIGN_MIDDLE;
    self->validate_button->validated = (EventListener){
        .callback = (EventListenerFunc)button_pressed,
        .target = self
    };

    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->text), 0, 0);
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->list),
        0,
        BASE_GAUGE(self->text)->frame.h + 3
    );

    /*lat/lon*/
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->latitude),
        SDLExt_RectLastX(&BASE_GAUGE(self)->frame) - BASE_GAUGE(self->latitude)->frame.w + 1,
        SDLExt_RectLastY(&BASE_GAUGE(self->list)->frame) + 3
    );
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->longitude),
        SDLExt_RectLastX(&BASE_GAUGE(self)->frame) - BASE_GAUGE(self->longitude)->frame.w + 1,
        SDLExt_RectLastY(&BASE_GAUGE(self->latitude)->frame) + 3
    );

    /*Bearing*/
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->bearing_lbl),
        0,
        SDLExt_RectLastY(&BASE_GAUGE(self->latitude)->frame)
        - (BASE_GAUGE(self->bearing_lbl)->frame.h-1)
    );
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->bearing_value),
        SDLExt_RectLastX(&BASE_GAUGE(self->bearing_lbl)->frame)+1,
        BASE_GAUGE(self->latitude)->frame.y
    );

    /*Distance*/
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->distance_lbl),
        0,
        SDLExt_RectLastY(&BASE_GAUGE(self->longitude)->frame)
        - (BASE_GAUGE(self->distance_lbl)->frame.h-1)
    );
    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->distance_value),
        SDLExt_RectLastX(&BASE_GAUGE(self->distance_lbl)->frame)+1,
        BASE_GAUGE(self->longitude)->frame.y
    );

    base_gauge_add_child(BASE_GAUGE(self),
        BASE_GAUGE(self->validate_button),
        0,
        SDLExt_RectLastY(&BASE_GAUGE(self->distance_lbl)->frame)+2
    );

    list_box_set_model(self->list, LIST_MODEL(airport_list_model_new()));

    self->visible = true;
    return self;
}

static void direct_to_dialog_render(DirectToDialog *self, Uint32 dt, RenderContext *ctx)
{
    base_gauge_fill(BASE_GAUGE(self), ctx, NULL, &SDL_BLACK, false);
//    base_gauge_render(BASE_GAUGE(self), dt, ctx);
}

static bool direct_to_dialog_handle_event(DirectToDialog *self, SDL_KeyboardEvent *event)
{
    /*TODO: Have a Dialog superclass that cycles through children?
     * Problem: we'll need some type of RTTI + ancestry or have a second
     * set of children referencing only BaseWidget derivatives
     * */
    bool keep_focus = base_widget_handle_event(self->focused, event);
    if(!keep_focus){
        self->focused->has_focus = false;

        if(self->focused == BASE_WIDGET(self->text))
            self->focused = BASE_WIDGET(self->list);
        else if(self->focused == BASE_WIDGET(self->list))
            self->focused = BASE_WIDGET(self->validate_button);
        else if(self->focused == BASE_WIDGET(self->validate_button))
            self->focused = BASE_WIDGET(self->text);


        self->focused->has_focus = true;
    }
    return true;
}

static void update_list_content(TextBox *txtbx, DirectToDialog *self)
{
    airport_list_model_filter((AirportListModel*)self->list->model, txtbx->text);
}

static void selection_changed(DirectToDialog *self, ListBox *sender)
{
    DataSource *ds;
    GeoLocation me, ap;

    ListModelRow *row = list_box_get_selected(sender);
    Airport *airport = (Airport*)row->key;

    geo_location_latitude_to_dms(airport->latitude, self->latitude->value);
    self->latitude->len = strlen(self->latitude->value);
//    printf("self->latitude->value: %s\n",self->latitude->value);
    BASE_GAUGE(self->latitude)->dirty = true;

    geo_location_longitude_to_dms(airport->longitude, self->longitude->value);
    self->longitude->len = strlen(self->longitude->value);
    BASE_GAUGE(self->longitude)->dirty = true;

    ds = data_source_get();
    if(!ds) return;

    me = (GeoLocation){ds->latitude, ds->longitude};
    ap = (GeoLocation){airport->latitude, airport->longitude};

    double distance = geo_location_distance_to(&me, &ap);
    distance /= 1852; /*convert to NM*/
    /* 4-digits are enough to represent the longest NM distance
     * between two points on earth */
    text_gauge_set_value_formatn(self->distance_value, 4, "%d", (int)round(distance));

    double bearing = geo_location_bearing(&me, &ap);
    text_gauge_set_value_formatn(self->bearing_value, 4, "%d\x8f", (int)round(bearing));
#if 0
    printf("Current selection: code: %s name: %s latitude: %f "
        "longitude: %f elevation: %d\n",
        airport->code,
        airport->name,
        airport->latitude,
        airport->longitude,
        airport->elevation
    );
#endif
}


static void button_pressed(DirectToDialog *self, Button *sender)
{
    ListModelRow *row = list_box_get_selected(self->list);
    Airport *airport = (Airport*)row->key;

    printf("Current selection: code: %s name: %s latitude: %f "
        "longitude: %f elevation: %d\n",
        airport->code,
        airport->name,
        airport->latitude,
        airport->longitude,
        airport->elevation
    );

    self->visible = false;
}
