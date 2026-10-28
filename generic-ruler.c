/*
 * SPDX-FileCopyrightText: 2021-2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "generic-ruler.h"

#include "SDL_pcf.h"
#include "generic-strip.h"
#include "misc.h"
#include "ruler-core.h"

/**
 * GenericRuler builds on GenericStrip to provide reusable features for handling
 * cases where:
 * a) a range of values is mapped to a pixel range
 * b) that range of values have one or more level of graduations (tick marks)
 * at regular intervals.
 *
 * It can be initialized in two ways:
 * a) The value range is known (e.g from 0 to 100) and the graduation interval
 * is known (value-wise), e.g. a tick every 10 units. The object will compute a
 * matching number of pixels to represent this interval of 10 units. Withing
 * the ruler size. A ruler that is 500px of size will not represent the same 10
 * units with the same number of pixels than a ruler that spans 350px. This mode
 * uses generic_ruler_init_from_size.
 * b) The graduation interval is known (e.g. 10 units) and the number of pixels
 * that represent this interval is known (e.g. 50px). The object will compute the
 * value range that can be represented with the given size of the ruler.
 * This mode uses generic_ruler_init.
 *
 *
 * Either way, the approximate total size of the final GenericRuler must be
 * passed-in. It will be adjusted to accomodate for the markings that can be
 * drawn around the ruler itself.
 *
 * GenericRuler provides the features needed to resolve a value to a pixel
 * position and vice-versa.
 *
 * Callers are responsible for the sequence of initing the object and using the
 * provided functions to draw what's needed for their use case like ticks,
 * markings, color zones, etc.
 *
 * GenericRuler can use two coordinates systems: The canvas or image coordinates
 * that goes from 0 to the canvas width/height and the ruler coordinates
 * that goes from 0 to the ruler width/height. The ruler coordinates are used
 * when we need a ruler that is smaller than the canvas to we have enough room
 * to write the markings around it.
 *
 * GenericRuler uses an anchor point to map a value (that must be a multiple of
 * the base unit) to a pixel position (ruler coordinates).
 * The anchor point is defined by a value and a pixel position : Let's say that
 * the value "0" is in the middle of the ruler, which will be for a 101px ruler
 * at pixel 50. Given that, the GenericRuler will compute the first tick
 * position (leftmost for a horizontal GrowAlongAxis ruler) and store it.
 *
 *
 * TODO: Add a discussion on the Ops structure and how it allows for
 * customization of the drawing behavior.
 */


#define DEBUG_GENERIC_RULER 0

static GenericRulerOps generic_ruler_ops = {
    .draw_tick_label = NULL, /* will call ruler_core_draw_tick_label_impl by default */
    .will_draw_tick = NULL,
};

/* private forward declarations */
static bool generic_ruler_will_draw_tick(RulerCore *self, GenericStrip *strip,
                                         float value, int *tick_size, Uint32 *color);
static bool generic_ruler_draw_tick_label(RulerCore *self, GenericStrip *strip,
                                          float value, uint8_t precision,
                                          PCF_Font *font, Uint32 color,
                                          int x, int y, PCF_TextPlacement placement);
static int generic_ruler_color_zone_location_cmp(const void *a, const void *b);
static int generic_ruler_color_zone_divider_cmp(const void *a, const void *b);

/**
 * @brief Init a ruler that maps a range that goes from @p start (included) to
 * @p end (included) to a pixel region that can be either vertical or horizontal.
 * The region maximum size is specified with @p ruler_w and @p ruler_h.
 * Depending on the ruler direction, one of these two dimensions will be mapped
 * to the value range. It can be shrinked to match the nearest multiple of @p
 * step so that the last hatch mark lands on the last pixel of the range.
 *
 * The ruler can visually (@p direction) go either way, but the range must
 * always be specified as from being lower than to.
 *
 * @warning @p step must be either -1 (one big step from @p start to @p end) or
 * a divider of @p end, e.g:
 * from     to   step   valid
 *  0       25    5      yes  25 = 5*5  -> 5 marks, interval walked in 5 steps
 *  0       25    1      yes  25 = 25*1 -> 25marks, interval walked in 25 steps
 *  0       25    2      NO   25 = 12.5 * 2 -> not a integral number of marks
 *
 * @param self the GenericRuler to work on.
 * @param orientation Whether the ruler is horizontal or vertical.
 * @param direction Whether the ruler values get bigger when the axis value
 * increases (RulerGrowAlongAxis) e.g. if values get bigger when y increases
 * the ruler is oriented top-down or against it (RulerGrowAgainstAxis).
 * @param start First (lower) value of the range to map.
 * @param end Last(highier) value of the range to map
 * @param step Marking interval. e.g. from = 0, to = 25, step = 5 will
 * map a range from 0 to 25 with marks at every 5 units to @p ruler_w pixels if
 * @p orientation is RulerHorizontal and to @p ruler_h pixels if orientation is
 * RulerVertical. A negative value will disable intermediate graduations.
 * @param tick_size Size of the tick marks in pixels. The size is measured from
 * the spine to the end of the tick mark.
 * @param markings_font The PCF_Font that will be used for markings. NULL If no
 * markings are to be drawn.
 * @param markings_location Where the markings will be drawn, used to compute the
 * full dimensions needed by the ruler itself plus its eventual markings. Pass
 * LocationNone if you aren't goint to use the labels-drawing features of GenericRuler.
 * @param text_margin How many pixels to leave between the ruler and the markings.
 * When unsure, use GENERIC_RULER_DEFAULT_TEXT_MARGIN.
 * @param markings_precision How many digital places will be written for
 * each marking. 0 For integral part only.
 * @param ruler_w Maximum width of the actual drawn ruler, can be modified to
 * represent the range and accomodate for markings.
 * @param ruler_h Maximum height of the actual drawn ruler, can be modified to
 * represent the range and accomodate for markings
 *
 * @warning ruler_w and ruler_h are the (maximum) dimensions of the drawn ruler
 * itself (that can be shrinked to match the number of pixels available for
 * the range), not the dimensions of the whole GenericRuler. The whole
 * GenericRuler will be bigger as it will accomodate for the markings that can
 * be drawn around the ruler itself (if any). The final size of the object canvas
 * can be read from GENERIC_LAYER(self)->canvas->w and
 * GENERIC_LAYER(self)->canvas->h
 *
 * @see generic_ruler_draw_ticks
 * @see generic_ruler_draw_labels
 * @see generic_ruler_init_from_size
 *
 * TODO: Consider splitting out Sophie's algorithm. Use that plus the call to
 * generic_ruler_get_size_request and make this function call generic_ruler_init
 */
GenericRuler *generic_ruler_init_from_size(GenericRuler *self,
                                           RulerOrientation orientation,
                                           RulerDirection direction,
                                           float start, float end, float step,
                                           uint8_t tick_size,
                                           PCF_Font *markings_font,
                                           Location markings_location,
                                           int text_margin,
                                           int8_t markings_precision,
                                           int ruler_w, int ruler_h)
{
    int fullw, fullh;

    self->ops = &generic_ruler_ops;

    GENERIC_STRIP(self)->flags = (StripFlags)(orientation | direction);
    self->core.ruler_area.w = ruler_w;
    self->core.ruler_area.h = ruler_h;

    GENERIC_STRIP(self)->start = start;
    GENERIC_STRIP(self)->end = end;
    if(GENERIC_STRIP(self)->end < GENERIC_STRIP(self)->start){
        printf("%s: self->end(%f) must be greather than self->start(%f).",
            __FUNCTION__, GENERIC_STRIP(self)->end, GENERIC_STRIP(self)->start
        );
        return NULL;
    }

    self->core.nticks = 1;
    self->core.ticks[0].value = (step > 0) ? step : GENERIC_STRIP(self)->end;

    if(fmodf(GENERIC_STRIP(self)->end, self->core.ticks[0].value) != 0){
        printf("GenericRuler: Warning: range end (%f) is not a multiple of self->ticks[0].value(%f), rendering will break\n",
           GENERIC_STRIP(self)->end,
           self->core.ticks[0].value
        );
    }

    /*Sophie's algorithm*/
    int nintervals = (GENERIC_STRIP(self)->end - GENERIC_STRIP(self)->start) / self->core.ticks[0].value;
    int nbars = nintervals + 1;
    int pix_available = (generic_ruler_orientation(self, RulerHorizontal)) ?
                          ruler_w - (nbars*1) // each bar is 1px wide/tall
                        : ruler_h - (nbars*1);
    int interval_pix_size = pix_available/nintervals; //This is the best interval(number of pixels BETWEEN two ticks) size in px.
    self->core.ticks[0].stride = interval_pix_size + 1; //The stride is the increment in pixels that lands on the next ticks, hence +1

    /* interval pix size is the number of pixels between bars,
     * +1 includes the closing bar */
    GENERIC_STRIP(self)->ppv = (self->core.ticks[0].stride) / self->core.ticks[0].value;
    if(generic_ruler_orientation(self, RulerHorizontal))
        self->core.ruler_area.w = interval_pix_size * nintervals + (nbars*1); //Each bar is 1px wide
    else
        self->core.ruler_area.h = interval_pix_size * nintervals + (nbars*1);

    if(markings_font != NULL && markings_location != LocationNone){
        generic_ruler_get_size_request(self, markings_precision, markings_font, markings_location, text_margin, &fullw, &fullh);
    }else{
        fullw = self->core.ruler_area.w;
        fullh = self->core.ruler_area.h;
    }

    self->core.ticks[0].size = tick_size;
    self->core.ticks[0].labeled = true;

    //That needs ruler_area to be set before calling ruler_core_set_anchor
    ruler_core_set_anchor(&(self->core), GENERIC_STRIP(self), (Anchor){
        .position = 0,
        .value = generic_ruler_direction(self, RulerGrowAlongAxis)
                 ? GENERIC_STRIP(self)->start
                 : GENERIC_STRIP(self)->end
    });

    generic_layer_init(GENERIC_LAYER(self), fullw, fullh);

    return self;
}

/**
 * @brief Inits a ruler that will use graduations (tick marks) of known pixel size
 * (e.g. 5 will be represented with 11 pixels) and optional multiples, e.g.
 * a small tick for the base unit (that 5 in the example), a taller tick every 10
 * units and and a slightly taller tick every 15.
 *
 * @param self the GenericRuler to work on.
 * @param width width of the ruler (won't be modified vs @p ruler_w in
 * generic_ruler_init_from_size)
 * @param height height of the ruler (won't be modified)
 * @param orientation Whether the ruler is horizontal or vertical.
 * @param direction Whether the ruler values get bigger when the axis value
 * increases (RulerGrowAlongAxis) or the opposite (RulerGrowAgainstAxis).
 * @param base_unit The value of the base unit, e.g. 5.
 * @param base_unit_px The number of pixels that represent the base unit,
 * e.g. 11px.
 * @param base_unit_size The size (over the spine) of the base unit tick
 * mark in pixels, e.g. 5px.
 * @param base_unit_labeled Whether the base unit tick mark will get a marking.
 * @param nmultiples The size of the following consecutives RulerUnitMultiples,
 * 0 when none.
 * @param multiples Address of the first RulerUnitMultiple in a series of
 * @p nmultiples that will be used to represent how multiples of the base unit
 * will be drawn. The first element of the array will be used to represent the
 * first multiple of the base unit (@p base_unit), the second element will be
 * used to represent the second multiple of the base unit, and so on. NULL when
 * no multiples are to be used.
 * @param anchor The anchor point, where a value is mapped to a pixel position,
 * relative to the ruler area. The anchor value must be a multiple of the base unit.
 *
 * @see generic_ruler_draw_ticks
 * @see generic_ruler_draw_labels
 * @see generic_ruler_init_from_size
 */
GenericRuler *generic_ruler_init(GenericRuler *self,
                                  int width, int height,
                                  RulerOrientation orientation,
                                  RulerDirection direction,
                                  Location spine_location,
                                  float base_unit, uint16_t base_unit_px,
                                  uint8_t base_unit_size, bool base_unit_labeled,
                                  uint8_t nmultiples, RulerUnitMultiple *multiples,
                                  Anchor anchor)
{
    int axis_max;
    if(orientation == RulerVertical)
        assert(anchor.position < height);
    else
        assert(anchor.position < width);
    assert(fmodf(anchor.value,base_unit) == 0); //Anchor must be a multiple of the base unit

    axis_max = orientation == RulerVertical ? height-1 : width-1;
    generic_layer_init(GENERIC_LAYER(self), width, height);
    self->ops = &generic_ruler_ops;

    GENERIC_STRIP(self)->flags = (StripFlags)(orientation | direction);
    GENERIC_STRIP(self)->ppv = base_unit_px/base_unit;
    /*regardless of the orientation, start/end will always be the lower/higher value*/
    if(direction == RulerGrowAlongAxis){
        GENERIC_STRIP(self)->start = anchor.value - anchor.position/GENERIC_STRIP(self)->ppv;
        GENERIC_STRIP(self)->end = anchor.value + (axis_max-anchor.position)/GENERIC_STRIP(self)->ppv;
    }else{ //RulerGrowAgainstAxis
        GENERIC_STRIP(self)->start = anchor.value - (axis_max-anchor.position)/GENERIC_STRIP(self)->ppv;
        GENERIC_STRIP(self)->end = anchor.value + anchor.position/GENERIC_STRIP(self)->ppv;
    }
    self->core.ruler_area = (SDL_Rect){
        /*auto-zero x and y*/
        .w = width,
        .h = height
    };

    bool rv = ruler_core_set_ticks(
        &(self->core), base_unit, base_unit_px, base_unit_size,
        base_unit_labeled, nmultiples, multiples
    );
    if(!rv){
        printf("%s: FATAL: nmultiples(%d) must be <= 4. Change compile-time const in generic-ruler.h and rebuild\n", __FUNCTION__, nmultiples);
        exit(EXIT_FAILURE);
    }

    self->core.spine_coord = ruler_core_get_spine_coord(&(self->core), orientation, spine_location);

    ruler_core_set_anchor(&(self->core), GENERIC_STRIP(self), anchor);

    return self;
}

/**
 * @brief Release resources held by @p self.
 *
 * @param self a GenericRuler
 */
void generic_ruler_dispose(GenericRuler *self)
{
    generic_layer_dispose(GENERIC_LAYER(self));
}

/**
 * @brief compute how much an area has to grow to accomodate the ruler itself plus
 * the markings aroud it.
 *
 * @warning This functions modifies self->ruler_area and should be renamed.
 *
 * @param self a GenericRuler
 * @param font a The PCF_Font that'll be used for markings
 * @param markings_location Where the markings will be put relatively to
 * the ruler
 * @param text_margin How many pixels to leave between the ruler and the markings.
 * When unsure, use GENERIC_RULER_DEFAULT_TEXT_MARGIN.
 * @param w Where to put the computed needed width
 * @param h Where to put the computed needed height
 * @return true on success, false otherwise
 *
 */
#if __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
bool generic_ruler_get_size_request(GenericRuler *self, int8_t precision,
                                    PCF_Font *font, Location markings_location,
                                    int text_margin, int *w, int *h)
{
    int pixel_increment = roundf(self->core.ticks[0].value * GENERIC_STRIP(self)->ppv);
    int glyph_height = PCF_FontCharHeight(font);

    char vbuffer[10]; /*9999999999 or 999999999 with a floating dot*/
    SDL_Rect m_rect; //mark rect

    /*Pixels to add on each side of the ruler to accomodate labels*/
    int left, right;
    int top, bottom;

    int fbw = self->core.ruler_area.w;
    int fbh = self->core.ruler_area.h;

    left = right = top = bottom = 0;
    if(generic_ruler_orientation(self, RulerHorizontal)){
        if(generic_ruler_direction(self, RulerGrowAlongAxis)){
            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            left = SDLExt_RectMidX(&m_rect) - m_rect.x; //We need 'left' more pixels on the left

            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            right = SDLExt_RectLastX(&m_rect) - SDLExt_RectMidX(&m_rect);//We need 'right' more pixels on the right
        }else{ /* RulerGrowAgainstAxis */
            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            left = SDLExt_RectMidX(&m_rect) - m_rect.x; //We need 'left' more pixels on the left

            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            right = SDLExt_RectLastX(&m_rect) - SDLExt_RectMidX(&m_rect);//We need 'right' more pixels on the right
        }

        if(markings_location == Top){
            top = text_margin + glyph_height;
        }else if (markings_location == Bottom){
            bottom = text_margin + glyph_height;
        }else{
            printf("GenericRuler: Unsupported Etching location for Horizontal orientation\n");
            return false;
        }
    }else{ /* RulerVertical */
        if(generic_ruler_direction(self, RulerGrowAlongAxis)){
            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            top = SDLExt_RectMidY(&m_rect) - m_rect.y;

            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            bottom = SDLExt_RectLastY(&m_rect) - SDLExt_RectMidY(&m_rect);
        }else{ /* RulerGrowAgainstAxis */
            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            bottom = SDLExt_RectLastY(&m_rect) - SDLExt_RectMidY(&m_rect);

            snprintf(vbuffer, 10, "%*g", precision, GENERIC_STRIP(self)->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, false, &m_rect);
            top = SDLExt_RectMidY(&m_rect) - m_rect.y;
        }

        /* Assumes that self->end > self->start
         * means that it's also the lenghtiest to
         * write. It also just happens that we've
         * just measured it's size in m_rect.
         * */
        if(markings_location == Left){
            left = m_rect.w + text_margin;
        }else if (markings_location == Right) {
            right = m_rect.w + text_margin;
        }else{
            printf("GenericRuler: Unsupported Etching location for Vertical orientation\n");
            return false;
        }
    }

    *w = fbw + left + right;
    *h = fbh + top + bottom;

    self->core.ruler_area = (SDL_Rect){
        .x = left,
        .y = top,
        /* Pretty useless, this is just
         * re-setting to the same value
         * as fbw/fbh has been set from
         * these.*/
        .w = fbw,
        .h = fbh
    };
    return true;
}
#if __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif

/**
 * @brief Write ruler ticks(graduations) on the underlying canvas.
 *
 * @param self a GenericRuler
 * @param color The color to use for hatches, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine. Valid locations for
 * Horizontal gauges are Top,Center,Bottom and Left, Center, Right for Vertical
 * gauges. Ignored when @p spine is false.
 *
 * @return true on success, false otherwise.
 */
bool generic_ruler_draw_ticks(GenericRuler *self, Uint32 color,
                              bool etch_spine, Location spine_location)
{
    return ruler_core_draw_ticks(
        &(self->core), GENERIC_STRIP(self), color, etch_spine, spine_location,
        generic_ruler_will_draw_tick
    );
}

/**
 * @brief Draw labels (numbers) on the ruler at the predefined
 * step
 *
 * @param self a GenericRuler
 * @param location Where to put the labels. Valid locations for Horizontal
 * rulers are: Top, Bottom while valid locations for Vertical gauges are
 * Left and Right.
 * @param text_margin How many pixels to leave between the ruler and the labels.
 * When unsure, use RULER_CORE_DEFAULT_TEXT_MARGIN.
 * @param font The PCF_Font to use. Must be the same used when calling
 * generic_ruler_get_size_request
 * @param color The color to use, in the underlying surface format
 * @param precision How many digital places to write. 0 for integral part only.
 *
 * @return true on success, false otherwise.
 *
 * @see generic_ruler_get_size_request
 */
bool generic_ruler_draw_labels(GenericRuler *self, Location markings_location,
                               int text_margin, PCF_Font *font,
                               Uint32 color, uint8_t precision)
{
    return ruler_core_draw_labels(
        &(self->core), GENERIC_STRIP(self), markings_location, text_margin,
        font, color, precision, generic_ruler_draw_tick_label
    );
}

/* private helper functions */

/**
 * @brief Trampoline adapter function to call the will_draw_tick operation of
 * the GenericRulerOps with the correct arguments.
 *
 * @see WillDrawTickFunc
 */
static bool generic_ruler_will_draw_tick(RulerCore *self, GenericStrip *strip,
                                         float value, int *tick_size, Uint32 *color)
{
    GenericRuler *ruler;

    ruler = container_of(self, GenericRuler, core);
    return ruler->ops->will_draw_tick
           ? ruler->ops->will_draw_tick(ruler, value, tick_size, color)
           : true;
}

/**
 * @brief Trampoline adapter function to call the draw_tick_label operation of
 * the GenericRulerOps with the correct arguments.
 *
 * @see DrawTickLabelFunc
 */
static bool generic_ruler_draw_tick_label(RulerCore *self, GenericStrip *strip,
                                          float value, uint8_t precision,
                                          PCF_Font *font, Uint32 color,
                                          int x, int y, PCF_TextPlacement placement)
{
    GenericRuler *ruler;

    ruler = container_of(self, GenericRuler, core);
    return ruler->ops->draw_tick_label
           ? ruler->ops->draw_tick_label(ruler, value, precision, font, color, x, y, placement)
           : ruler_core_draw_tick_label_impl(self, strip, value, precision, font, color, x, y, placement);
}
