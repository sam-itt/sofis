#include <stdint.h>
#include <stdlib.h>

#include "inset-softkey-model.h"
#include "pfd-toplevel-softkey-model.h"
#include "softkey-model.h"
#include "softkey.h"

static SoftkeyDetails pfd_toplevel_softkeys[] = {
    {
        .caption = "INSET",
        .type = SOFTKEY_TYPE_SEGUE,
        .action.next = NULL
    },
    {
        .caption = "PFD",
    },
    {
        .caption = "OBS",
    },
    {
        .caption = "CDI",
    },
    {
        .caption = "DME",
    },
    {
        .caption = "XPDR",
    },
    {
        .caption = "IDENT",
    },
    {
        .caption = "TMR/REF",
    },
    {
        .caption = "NRST",
    },
    {
        .caption = "ALERTS",
    },
};


static PfdToplevelSoftkeyModel *pfd_toplevel_softkey_model_new();
static PfdToplevelSoftkeyModel *pfd_toplevel_softkey_model_init(PfdToplevelSoftkeyModel *self);
static SoftkeyDetails *pfd_toplevel_softkey_model_get_details_at(SoftkeyModel *self, uint_fast8_t index);
static SoftkeyModelOps pfd_toplevel_softkey_model_ops = {
//    .dispose = (DisposeFunc)airport_list_model_dispose
    .get_details_at = pfd_toplevel_softkey_model_get_details_at
};


static PfdToplevelSoftkeyModel *instance = NULL;


PfdToplevelSoftkeyModel *pfd_toplevel_softkey_model_get_instance(void)
{
    if(!instance){
        instance = pfd_toplevel_softkey_model_new();
    }
    return instance;
}

static PfdToplevelSoftkeyModel *pfd_toplevel_softkey_model_new()
{
    PfdToplevelSoftkeyModel *self;

    self = calloc(1, sizeof(PfdToplevelSoftkeyModel));
    if(self){
        if(!pfd_toplevel_softkey_model_init(self)){
            return NULL;
        }
    }
    return self;
}

static PfdToplevelSoftkeyModel *pfd_toplevel_softkey_model_init(PfdToplevelSoftkeyModel *self)
{
    softkey_model_init(SOFTKEY_MODEL(self), &pfd_toplevel_softkey_model_ops);
    pfd_toplevel_softkeys[0].action.next = SOFTKEY_MODEL(inset_softkey_model_get_instance());

    return self;
}

static SoftkeyDetails *pfd_toplevel_softkey_model_get_details_at(SoftkeyModel *self, uint_fast8_t index)
{
    if(index == 0 || index == 2) return NULL;

    if(index == 1) return &(pfd_toplevel_softkeys[0]);

    /* Plus one to skip the first storage array element which is button index 1 (second one)
     * minus 3 to "rebase" the index: All buttons are stored in order starting button 3 (4th one)
     * */
    return &(pfd_toplevel_softkeys[index+1-3]);
}

static void pfd_toplevel_softkey_model_inset_button_pressed(SoftkeyModel *self, Softkey *button)
{
    void *softkeybar = BASE_GAUGE(button)->parent;

    //softkey_bar_set_model(pdf_inset_softkey_model_get_instance());
}
