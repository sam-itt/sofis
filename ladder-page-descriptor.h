#ifndef LADDER_PAGE_DESCRIPTOR_H
#define LADDER_PAGE_DESCRIPTOR_H

#include "ruler-core.h"

typedef struct _LadderPage LadderPage;

typedef enum {TOP_DOWN, BOTTUM_UP} ScrollType;

/**
 * @brief Function pointer type for initializing a LadderPage. This function is
 * will be called after the LadderPage has been allocated. It should chain-up to
 * parent ladder_page_init to have the canvas allocated and its bounds set.
 *
 * At the moment this mechanism is used rather than a LadderPageOps struct to
 * lower complexity. If the need arises to have allocation and deallocations we
 * will move in that direction.
 *
 * @param self The blank LadderPage to initialize.
 *
 * @return The initialized LadderPage.
 */
typedef LadderPage *(*LPInitFunc) (LadderPage *self);

typedef struct _LadderPageDescriptor {
    RulerCore super;

    int width; /**< Page width in pixels. */
    int height; /**< Page height in pixels. */

    ScrollType direction; /**< The direction in which values grow (BOTTUM_UP or TOP_DOWN). */
    int offset; /**< leading/trailing pixels computed by the init function. */
    size_t page_size; /**< Number of values per page. */

    LPInitFunc init_page;
}LadderPageDescriptor;

#define LADDER_PAGE_DESCRIPTOR(self) ((LadderPageDescriptor*)(self))

LadderPageDescriptor *ladder_page_descriptor_init(LadderPageDescriptor *self, int width, int height,
                                                  ScrollType direction, float base_unit, uint16_t base_unit_px,
                                                  uint8_t base_unit_size, bool base_unit_labeled,
                                                  uint8_t nmultiples, RulerUnitMultiple *multiples,
                                                  LPInitFunc func);

LadderPage *ladder_page_descriptor_create_page(LadderPageDescriptor *self, int index);

/**
 * @brief Resolves the index of the page holding a given a value.
 *
 * @param self The LadderPageDescriptor
 * @param value The value to find the page index for.
 *
 * @return The page index that contains the given value.
 */
static inline int ladder_page_descriptor_get_idx_for(LadderPageDescriptor *self, float value)
{
    return (int)value/(self->page_size);
}



#endif /* LADDER_PAGE_DESCRIPTOR_H */
