#ifndef ALT_LADDER_PAGE_DESCRIPTOR_H
#define ALT_LADDER_PAGE_DESCRIPTOR_H

#include "ladder-page.h"
#include "fb-page-descriptor.h"

typedef struct{
    FBPageDescriptor parent;
}AltLadderPageDescriptor;


AltLadderPageDescriptor *alt_ladder_page_descriptor_new(void);
LadderPage *alt_ladder_page_init(LadderPage *self);
#endif /* ALT_LADDER_PAGE_DESCRIPTOR_H */
