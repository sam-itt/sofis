#include <stdint.h>
#include <stdlib.h>

#include "inset-softkey-model.h"
#include "softkey-model.h"
#include "softkey.h"

static SoftkeyDetails inset_softkeys[] = {
    {
        .caption = "OFF",
    },
    {
        .caption = "DCLTR",
    },
    {
        .caption = "WXLGND",
    },
    {
        .caption = "TRAFFIC",
    },
    {
        .caption = "TOPO",
    },
    {
        .caption = "TERRAIN",
    },
    {
        .caption = "STRMSCP",
    },
    {
        .caption = "NEXRAD",
    },
    {
        .caption = "XM LTNG",
    },
    {
        .caption = "METAR",
    },
    {
        .caption = "BACK",
        .type = SOFTKEY_TYPE_BACK
    },
    {
        .caption = "ALERTS",
    },
};


static InsetSoftkeyModel *inset_softkey_model_new();
static InsetSoftkeyModel *inset_softkey_model_init(InsetSoftkeyModel *self);
static SoftkeyDetails *inset_softkey_model_get_details_at(SoftkeyModel *self, uint_fast8_t index);
static SoftkeyModelOps inset_softkey_model_ops = {
//    .dispose = (DisposeFunc)airport_list_model_dispose
    .get_details_at = inset_softkey_model_get_details_at
};

static InsetSoftkeyModel *instance = NULL;


InsetSoftkeyModel *inset_softkey_model_get_instance(void)
{
    if(!instance){
        instance = inset_softkey_model_new();
    }
    return instance;
}

static InsetSoftkeyModel *inset_softkey_model_new()
{
    InsetSoftkeyModel *self;

    self = calloc(1, sizeof(InsetSoftkeyModel));
    if(self){
        if(!inset_softkey_model_init(self)){
            return NULL;
        }
    }
    return self;
}

static InsetSoftkeyModel *inset_softkey_model_init(InsetSoftkeyModel *self)
{
    softkey_model_init(SOFTKEY_MODEL(self), &inset_softkey_model_ops);

    return self;
}

static SoftkeyDetails *inset_softkey_model_get_details_at(SoftkeyModel *self, uint_fast8_t index)
{
    return &(inset_softkeys[index]);
}

static void inset_softkey_model_inset_button_pressed(SoftkeyModel *self, Softkey *button)
{
    void *softkeybar = BASE_GAUGE(button)->parent;

    //softkey_bar_set_model(pdf_inset_softkey_model_get_instance());
}
