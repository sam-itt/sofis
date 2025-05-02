/*
 * SPDX-FileCopyrightText: 2025 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdlib.h>

#include "softkey-model.h"


SoftkeyModel *softkey_model_init(SoftkeyModel *self, SoftkeyModelOps *ops)
{
    self->ops = ops;

    return self;
}

SoftkeyModel *softkey_model_dispose(SoftkeyModel *self)
{
    if(self->ops->dispose)
        self->ops->dispose(self);

    return self;
}


SoftkeyDetails *softkey_model_get_details_at(SoftkeyModel *self, uint_fast8_t index)
{
    if(index < 0 || index > 11) return NULL;

    return self->ops->get_details_at(self, index);
}

