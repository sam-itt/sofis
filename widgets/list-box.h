#ifndef LIST_BOX_H
#define LIST_BOX_H
#include "SDL_pcf.h"

#include "base-widget.h"
#include "resource-manager.h"
#include "list-model.h"

#ifndef SDLEXT_UPOINT
#define SDLEXT_UPOINT
typedef struct{
    Uint32 x;
    Uint32 y;
}SDLExt_UPoint;
#endif

typedef struct{
    PCF_StaticFontRectPatch *patches;
    size_t apatches;
    size_t npatches;
    Uint32 selected_y;
    Uint32 selected_h;

    SDLExt_UPoint offset; /*offset in the virtual rectangle*/
}ListBoxState;

typedef struct _ListBox{
    BaseWidget super;

    uint_fast8_t font_size; /*RFU*/
    PCF_StaticFont *sfont;

    ListModel *model;
    size_t selected_row;

    SDL_Rect text_size; /*Virtual rectangle with the whole text*/

    ListBoxState state;

    EventListener selection_changed;
}ListBox;


ListBox *list_box_new(FontResource font_id, int width, int height);
ListBox *list_box_init(ListBox *self, FontResource font_id, int width, int height);

void list_box_set_model(ListBox *self, ListModel *model);
void list_box_model_changed(ListBox *self);

bool list_box_vertical_scroll(ListBox *self, int_fast8_t direction);
bool list_box_horizontal_scroll(ListBox *self, int_fast8_t direction);

static inline ListModelRow *list_box_get_selected(ListBox *self)
{
    return &self->model->rows[self->selected_row];
}

static inline void list_box_set_selection_changed_listener(ListBox *self, EventListenerFunc callback, void *target)
{
    self->selection_changed.callback = callback;
    self->selection_changed.target = target;
}

#endif /* LIST_BOX_H */
