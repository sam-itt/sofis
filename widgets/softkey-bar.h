#ifndef SOFTKEY_BAR_H_ /*Wihtout the trailing underscore guard*/
#define SOFTKEY_BAR_H_ /*would conflict with the height define*/

#include "base-widget.h"
#include "softkey.h"
#include "softkey-model.h"
#include "resource-manager.h"

#define N_SOFTKEYS 12

typedef struct {
    BaseWidget super;

    Softkey buttons[N_SOFTKEYS];

    SoftkeyModel *model;
}SoftkeyBar;


SoftkeyBar *softkey_bar_new(FontResource font_id, SoftkeyModel *model);
SoftkeyBar *softkey_bar_init(SoftkeyBar *self, SoftkeyModel *model, FontResource font_id, int w, int h);

void softkey_bar_set_model(SoftkeyBar *self, SoftkeyModel *model);

void softkey_bar_push_model(SoftkeyBar *self, SoftkeyModel *model);
bool softkey_bar_pop_model(SoftkeyBar *self);
#endif /* SOFTKEY_BAR_H_ */
