#ifndef INSET_SOFTKEY_MODEL_H
#define INSET_SOFTKEY_MODEL_H

#include "softkey-model.h"

typedef struct{
   SoftkeyModel super;
}InsetSoftkeyModel;


InsetSoftkeyModel *inset_softkey_model_get_instance(void);
#endif /* INSET_SOFTKEY_MODEL_H */
