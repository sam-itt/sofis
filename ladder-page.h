#ifndef LADDER_PAGE_H
#define LADDER_PAGE_H

#include "sdl-pcf/SDL_pcf.h"
#include "vertical-strip.h"

typedef struct _LadderPage LadderPage;

typedef enum {TOP_DOWN, BOTTUM_UP} ScrollType;
typedef LadderPage *(*LPInitFunc) (LadderPage *self);


/**
 * LadderPageDescriptor helps define how the inifinite
 * strip is made out of pages, or how pages are put together
 * into the inifinte strip.
 *
 * The strip is defined by a page dimension {@value page_size}
 * and a step between values {@value vstep} that tells the
 * LadderGauge how many values are in a page. i.e The first page
 * of a strip with a descriptor having a page of size of 70 with
 * a step of 10 will likely go from 0 to 69 (70 values). The image
 * is assumed to have marks every 10 values.
 *
 * The image must in fact be larger than that: 0 cannot be the very first
 * pixel, and 69 cannot be the very last: This wouldn't allow for
 *  a) putting a number in front of the mark: The digit being centered on
 *     the mark, there wouldn't be enough space for the lower/upper part as
 *     the digit centerline would be right at 0 or max-1
 *  b) chaining up the pages: the strip is built by concatenating pages
 *     one right after/before the other. Having the min/max marks right at
 *     0/max pixels would result in eating up an interval: Imagine page 0
 *     being *values* 0-9 and page 2 being *values* 10-19, with graduations
 *     every unit. It's safe to assume that the ruler image will have a fixed
 *     amount of pixels between each mark.
 *     A direct contactenation would have 9 and 10 with absolutely no interval
 *     in-between which doesn't work.
 * Therefore the image needs to be larger than the interval. It needs to have
 * a leading and trailing area that together account for the same space as in
 * between two value. i.e if value are 6 pixels appart, you need 3 leading pixels
 * and 3 trailing pixels. This will allow for seamless concatenatation.
 * WARNING: If it's not possible to have an equal amount of leading/trailing pixels,
 * LEADING area must get the HIGHER number of pixels. Example: 7 pixels spacing
 * between values, hence 3.5 pixels which cannot be done. 4 leading pixels, 3
 * trailing.
 *
 * Either if you use a loaded image or generate it on the fly, you must follow this
 * rule.
 *
 * The creation process will compute {@value offset} that will the be used to "move"
 * pages from the "nominal" interval (Page X starts at V) to the real interval that
 * includes the leading/trailing pixels (Page X has it's first mark at V but in fact
 * has pixels that maps to V-offset)
 *
 */
typedef struct{
    ScrollType direction;
    size_t page_size; /*number of values per page*/

    float fei; /*First etch mark index*/
    float vstep; /*Main unit etch marks*/
    float vsubstep; /*Subunit unit etch marks*/

    float offset; /*Trailing/leading pixels turned into value units*/

    LPInitFunc init_page;
}LadderPageDescriptor;


/**
 * Abstract struct that represent a VerticalStrip that will
 * be used as a page in a LadderGauge. The LadderGauge displays
 * a tape-like widget that scrolls an infinite slider. The
 * 'infinite' slider is made out of pages.
 *
 * Each page represents a range. All pages making up an infinite
 * slider share a common descriptor that helps understanding the
 * page within the tape and create new ones.
 *
 * Each page can be linked to an index within the strip. Page X
 * can be computed as started at value V.
 *
 */
struct _LadderPage{
    VerticalStrip super;

//    const LadderPageDescriptor *descriptor;
    LadderPageDescriptor *descriptor;
};

#define LADDER_PAGE(self) ((LadderPage*)(self))
#define LADDER_PAGE_DESCRIPTOR(self) ((LadderPageDescriptor*)(self))

LadderPageDescriptor *ladder_page_descriptor_init(LadderPageDescriptor *self, ScrollType direction, float page_size, float vstep, float vsubstep, LPInitFunc func);
void ladder_page_descriptor_compute_offset(LadderPageDescriptor *self, float ppv);



LadderPage *ladder_page_new(float start, LadderPageDescriptor *descriptor);
//LadderPage *ladder_page_init(LadderPage *self);
void ladder_page_free(LadderPage *self);

int ladder_page_get_index(LadderPage *self);
float ladder_page_resolve_value(LadderPage *self, float value);
void ladder_page_etch_markings(LadderPage *self, PCF_Font *font);
#endif /* LADDER_PAGE_H */
