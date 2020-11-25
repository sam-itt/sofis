#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <SDL2/SDL_image.h>

#include "fb-page-descriptor.h"
#include "generic-layer.h"

LadderPage *fb_ladder_page_init(LadderPage *self);

FBPageDescriptor *fb_page_descriptor_new(const char *filename, ScrollType direction, float page_size, float vstep, float vsubstep)
{
    FBPageDescriptor *self;

    self = calloc(1, sizeof(FBPageDescriptor));
    if(self){
        if(!fb_page_descriptor_init(self, filename, direction, page_size, vstep, vsubstep)){
            free(self);
            return NULL;
        }

    }
    return self;
}


FBPageDescriptor *fb_page_descriptor_init(FBPageDescriptor *self, const char *filename, ScrollType direction, float page_size, float vstep, float vsubstep)
{
    ladder_page_descriptor_init(
        LADDER_PAGE_DESCRIPTOR(self), direction,
        page_size, vstep, vsubstep,
        fb_ladder_page_init
    );
//    self->filename = strdup(filename); //TODO: strdup ?
    self->filename = (char*)filename; //TODO: strdup ?

    return self;
}

void fb_page_descriptor_dispose(FBPageDescriptor *self)
{
    free(self->filename);
}

LadderPage *fb_ladder_page_init(LadderPage *self)
{
    VerticalStrip *strip;
    GenericLayer *layer;
    LadderPageDescriptor *adesc;
    FBPageDescriptor *descriptor;

    strip = VERTICAL_STRIP(self);
    layer = GENERIC_LAYER(self);
    descriptor = (FBPageDescriptor *)LADDER_PAGE(self)->descriptor;
    adesc = LADDER_PAGE(self)->descriptor;

    bool rv;
    rv = generic_layer_init_from_file(GENERIC_LAYER(self),descriptor->filename);
    if(!rv){
        return NULL;
    }

    strip->ppv = generic_layer_h(layer)/(adesc->page_size*1.0);
//    printf("FileBacked on %s ppv is %f\n",descriptor->filename,strip->ppv);
//    printf("Page marking range is [%f, %f]\n", strip->start, strip->end);

    /* We are just going to offset the interval, size remains the same
     * so ppv wont change and can be computed before or afterwards*/

    if(isnan(adesc->offset)){
        ladder_page_descriptor_compute_offset(adesc, strip->ppv);
    }

    strip->start += adesc->offset;
    strip->end = strip->start + adesc->page_size-1;

//    int page_index = ladder_page_get_index(LADDER_PAGE(self));
//    printf("Page %d real range is [%f, %f]\n",page_index, strip->start, strip->end);

    return self;
}

void fb_ladder_page_dispose(LadderPage *self)
{
    vertical_strip_dispose(VERTICAL_STRIP(self));

}
