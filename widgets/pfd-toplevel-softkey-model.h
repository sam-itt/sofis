#ifndef PFD_TOPLEVEL_SOFTKEY_MODEL_H
#define PFD_TOPLEVEL_SOFTKEY_MODEL_H

#include "softkey-model.h"

typedef struct{
   SoftkeyModel super;
}PfdToplevelSoftkeyModel;


PfdToplevelSoftkeyModel *pfd_toplevel_softkey_model_get_instance(void);
#endif /* PFD_TOPLEVEL_SOFTKEY_MODEL_H */
