/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
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
 * passed-in. It will be adjusted to accomodate for the markings thati can be
 * etched around the ruler itself.
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
 * customization of the etching behavior.
 */



#define DEBUG_GENERIC_RULER 0

static GenericRulerOps generic_ruler_ops = {
    .etch_tick_label = generic_ruler_etch_tick_label_impl,
    .will_etch_tick = NULL,
};



static void generic_ruler_set_anchor(GenericRuler *self, Anchor anchor);
static bool generic_ruler_draw_hatches_vertical(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location, int preload_px);
static bool generic_ruler_draw_hatches_horizontal(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location, int preload_px);

static bool generic_ruler_etch_markings_vertical(GenericRuler *self, Location markings_location, int text_margin, PCF_Font *font, Uint32 color, uint8_t precision);
static bool generic_ruler_etch_markings_horizontal(GenericRuler *self, Location markings_location, int text_margin, PCF_Font *font, Uint32 color, uint8_t precision);


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
 * LocationNone if you aren't goint to use the markings-etching features of GenericRuler.
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
 * be etched around the ruler itself (if any). The final size of the object canvas
 * can be read from GENERIC_LAYER(self)->canvas->w and
 * GENERIC_LAYER(self)->canvas->h
 *
 * @see generic_ruler_etch_hatches
 * @see generic_ruler_etch_markings
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
    self->ruler_area.w = ruler_w;
    self->ruler_area.h = ruler_h;

    GENERIC_STRIP(self)->start = start;
    GENERIC_STRIP(self)->end = end;
    if(GENERIC_STRIP(self)->end < GENERIC_STRIP(self)->start){
        printf("%s: self->end(%f) must be greather than self->start(%f).",
            __FUNCTION__, GENERIC_STRIP(self)->end, GENERIC_STRIP(self)->start
        );
        return NULL;
    }

    self->nticks = 1;
    self->ticks[0].value = (step > 0) ? step : GENERIC_STRIP(self)->end;

    if(fmodf(GENERIC_STRIP(self)->end, self->ticks[0].value) != 0){
        printf("GenericRuler: Warning: range end (%f) is not a multiple of self->ticks[0].value(%f), rendering will break\n",
           GENERIC_STRIP(self)->end,
           self->ticks[0].value
        );
    }

    /*Sophie's algorithm*/
    int nintervals = (GENERIC_STRIP(self)->end - GENERIC_STRIP(self)->start) / self->ticks[0].value;
    int nbars = nintervals + 1;
    int pix_available = (generic_ruler_orientation(self, RulerHorizontal)) ?
                          ruler_w - (nbars*1) // each bar is 1px wide/tall
                        : ruler_h - (nbars*1);
    int interval_pix_size = pix_available/nintervals; //This is the best interval(number of pixels BETWEEN two etches) size in px.
    self->ticks[0].stride = interval_pix_size + 1; //The stride is the increment in pixels that lands on the next ticks, hence +1

    /* interval pix size is the number of pixels between bars,
     * +1 includes the closing bar */
    GENERIC_STRIP(self)->ppv = (self->ticks[0].stride) / self->ticks[0].value;
    if(generic_ruler_orientation(self, RulerHorizontal))
        self->ruler_area.w = interval_pix_size * nintervals + (nbars*1); //Each bar is 1px wide
    else
        self->ruler_area.h = interval_pix_size * nintervals + (nbars*1);

    if(markings_font != NULL && markings_location != LocationNone){
        generic_ruler_get_size_request(self, markings_precision, markings_font, markings_location, text_margin, &fullw, &fullh);
    }else{
        fullw = self->ruler_area.w;
        fullh = self->ruler_area.h;
    }

    self->ticks[0].size = tick_size;
    self->ticks[0].labeled = true;

    //That needs ruler_area to be set before calling generic_ruler_set_anchor
    generic_ruler_set_anchor(self, (Anchor){
        .position=0,
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
 * @see generic_ruler_etch_hatches
 * @see generic_ruler_etch_markings
 * @see generic_ruler_init_from_size
 */

GenericRuler *generic_ruler_init(GenericRuler *self,
                                  int width, int height,
                                  RulerOrientation orientation,
                                  RulerDirection direction,
                                  float base_unit, uint16_t base_unit_px,
                                  uint8_t base_unit_size, bool base_unit_labeled,
                                  uint8_t nmultiples, RulerUnitMultiple *multiples,
                                  Anchor anchor)
{
    if(orientation == RulerVertical)
        assert(anchor.position < height);
    else
        assert(anchor.position < width);
    assert(fmodf(anchor.value,base_unit) == 0); //Anchor must be a multiple of the base unit
                                                //
    generic_layer_init(GENERIC_LAYER(self), width, height);
    self->ops = &generic_ruler_ops;

    GENERIC_STRIP(self)->flags = (StripFlags)(orientation | direction);
    GENERIC_STRIP(self)->ppv = base_unit_px/base_unit;
    /*regardless of the orientation, start/end will always be the lower/higher value*/
    GENERIC_STRIP(self)->start = anchor.value - anchor.position/GENERIC_STRIP(self)->ppv;
    GENERIC_STRIP(self)->end = anchor.value + (height-1-anchor.position)/GENERIC_STRIP(self)->ppv;
    self->ruler_area = (SDL_Rect){
        /*auto-zero x and y*/
        .w = width,
        .h = height
    };

    if(nmultiples > 4){
        printf("%s: FATAL: nmultiples(%d) must be <= 4. Change compile-time const in generic-ruler.h and rebuild\n", __FUNCTION__, nmultiples);
        exit(EXIT_FAILURE);
    }
    self->nticks = nmultiples + 1;
    self->ticks[0].value = base_unit;
    self->ticks[0].stride = base_unit_px;
    self->ticks[0].size = base_unit_size;
    self->ticks[0].labeled = base_unit_labeled;
    for(int i = 0; i < nmultiples; i++){
        self->ticks[i+1].value = multiples[i].value * base_unit;
        self->ticks[i+1].stride = multiples[i].value * base_unit_px;
        self->ticks[i+1].size = multiples[i].size;
        self->ticks[i+1].labeled = multiples[i].labeled;
    }

    generic_ruler_set_anchor(self, anchor);

    return self;
}

/**
 * @brief Convert a pixel position within the GenericRuler canvas to a value
 * within the ruler's range. This function accepts a pixel position from 0 to
 * the canvas width/height (depending on the orientation) and returns the
 * corresponding value within the ruler's range. If the pixel position is
 * outside the actual ruler_rect portion of the canvas, the function
 * will return NAN.
 *
 * Contrary to generic_ruler_increment_to_value, this function accepts a pixel
 * absolute position within the canvas, not a pixel increment relative to the
 * ruler's start.
 *
 * @param self a GenericRuler
 * @param px_position The pixel position to convert.
 *
 * @return The corresponding value within the ruler's range, or NAN if the pixel
 * position is on the canvas but outside the ruler.
 *
 * @see generic_ruler_increment_to_value
 */
float generic_ruler_position_to_value(GenericRuler *self, unsigned int px_position)
{
    int axis_min, axis_max;

    axis_min = generic_ruler_axis_min(self);
    axis_max = generic_ruler_axis_max(self);

    if(px_position < axis_min || px_position > axis_max){
        printf("%s: px_position(%d) is out of bounds (min:%d, max:%d)\n", __FUNCTION__, px_position, axis_min, axis_max);
        return NAN;
    }

    return generic_ruler_increment_to_value(self, px_position - axis_min);
}

/**
 * @brief Convert a pixel increment (relative to the ruler's start) to a value
 * within the ruler's range.
 *
 * @param self a GenericRuler
 * @param px_increment The pixel increment to convert.
 *
 * @return The corresponding value within the ruler's range, or NAN if the pixel
 * increment is out of bounds.
 *
 * @see generic_ruler_position_to_value
 */
float generic_ruler_increment_to_value(GenericRuler *self, unsigned int px_increment)
{
    int delta;
    int axis_max;
    TickMark *base;

    base = &(self->ticks[0]);
    axis_max = generic_ruler_orientation(self, RulerHorizontal)
               ? self->ruler_area.w-1
               : self->ruler_area.h-1;

    if(px_increment > axis_max){
        printf("%s: px_increment(%d) is out of bounds (min:0, max:%d)\n", __FUNCTION__, px_increment, axis_max);
        return NAN;
    }

    if(px_increment == self->first_tick.position) return self->first_tick.value;

    delta = (px_increment >= self->first_tick.position)
          ? (px_increment - self->first_tick.position)
          : (self->first_tick.position - px_increment);
    if(delta % base->stride == 0){ /*are we on a tick mark?*/
        int nunits = delta/base->stride;
        /* That could be one line with appropriate inverting factors? */
        if(px_increment > self->first_tick.position){
            if(generic_ruler_direction(self, RulerGrowAlongAxis))
                return self->first_tick.value + nunits*base->value;
            else
                return self->first_tick.value - nunits*base->value;
        }else{
            if(generic_ruler_direction(self, RulerGrowAlongAxis))
                return self->first_tick.value - nunits*base->value;
            else
                return self->first_tick.value + nunits*base->value;
        }
    }
    /*otherwise we resort to approximation*/
    if(generic_ruler_direction(self, RulerGrowAlongAxis))
        return GENERIC_STRIP(self)->start + px_increment/GENERIC_STRIP(self)->ppv;
    else
        return GENERIC_STRIP(self)->start + (axis_max - px_increment)/GENERIC_STRIP(self)->ppv;
}

/**
 * @brief Write ruler hatches(graduations) on the underlying canvas.
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
bool generic_ruler_etch_hatches(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location)
{
    /*This first version ignores the orientation or direction, it will only work for vertical bottom-up*/

    TickMark base;
    Anchor anchor;
    int axis_max;


    base = self->ticks[0];
    anchor = self->first_tick;
    axis_max = generic_ruler_orientation(self, RulerHorizontal)
               ? self->ruler_area.w-1
               : self->ruler_area.h-1;

    /* We need our anchor to be a big graduation.
     * If that's an intermediate one (it's asserted to be a multiple
     * of the base unit earlier), let's walk some base units until we
     * find the closest big graduation. That will be our "virtual"
     * anchor point.
     * ...
     */
    /*TODO: Compute that directlty*/
    while(fmodf(anchor.value, self->ticks[self->nticks-1].value) != 0){
        anchor.value += base.value;
        anchor.position += generic_ruler_direction(self, RulerGrowAlongAxis)
                           ? base.stride
                           : -base.stride;
    }

    /* ... so we can start from the biggest graduation and determine how many
     * of them we have between the "virtual" anchor and the limit of the
     * drawing area.
     * We eat that up and we are left with the remaining pixels. Those pixels
     * are the end of another virtual big graduation that starts big grad size
     * minus remaining pixels before the drawing area. We just preload that
     * and continue from there and everything will line up
     */
    int preload_px = 0;
    int delta = axis_max - anchor.position;
    int nunits = delta/self->ticks[self->nticks-1].stride;
    int remaining = delta - nunits*self->ticks[self->nticks-1].stride;
    if(remaining){
        preload_px = self->ticks[self->nticks-1].stride - remaining;
    }
    return generic_ruler_draw_hatches(self, color, etch_spine, spine_location, preload_px);
}

/**
 * @brief Draw the hatches(graduations) on the ruler.
 *
 * This function does a pure graphical etch of the hatches, it does not consider
 * values. From pixel 0 to the end of the ruler, it will draw hatches at each
 * multiple of the TickMarks of @p self. @p preload_px allows to pretend that
 * some pixels have already been drawn, so that the hatches will be drawn as if
 * the ruler was already drawn up to that. See generic_ruler_etch_hatches for
 * an example of how to use this.
 *
 * @param self a GenericRuler
 * @param color The color to use for hatches, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine.
 * @param preload_px How much pixels to pretend to have already written.
 *
 * @return true on success, false otherwise.
 */
bool generic_ruler_draw_hatches(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location, int preload_px)
{
    return generic_ruler_orientation(self, RulerHorizontal)
           ? generic_ruler_draw_hatches_horizontal(self, color, etch_spine, spine_location, preload_px)
           : generic_ruler_draw_hatches_vertical(self, color, etch_spine, spine_location, preload_px);
}

/**
 * @brief Etch markings (numbers) on the ruler at the predefined
 * step
 *
 * @param self a GenericRuler
 * @param location Where to put the markings. Valid locations for Horizontal
 * rulers are: Top, Bottom while valid locations for Vertical gauges are
 * Left and Right.
 * @param text_margin How many pixels to leave between the ruler and the markings.
 * When unsure, use GENERIC_RULER_DEFAULT_TEXT_MARGIN.
 * @param font The PCF_Font to use. Must be the same used when calling
 * generic_ruler_get_size_request
 * @param color The color to use, in the underlying surface format
 * @param precision How many digital places to write. 0 for integral part only.
 *
 * @return true on success, false otherwise.
 *
 * @see generic_ruler_get_size_request
 */
bool generic_ruler_etch_markings(GenericRuler *self, Location markings_location, int text_margin, PCF_Font *font, Uint32 color, uint8_t precision)
{
    return generic_ruler_orientation(self, RulerHorizontal)
           ? generic_ruler_etch_markings_horizontal(self, markings_location, text_margin, font, color, precision)
           : generic_ruler_etch_markings_vertical(self, markings_location, text_margin, font, color, precision);
#if DEBUG_GENERIC_RULER
    generic_layer_save_bmp(GENERIC_LAYER(self), "/tmp/ruler-try-hatches.bmp");
#endif
}

/**
 * @brief This is the default implementation of the etch_tick_label operation, that
 * just writes the value at the given position using the appropriate font and color.
 *
 * Subclasses can override this operation using their version of GenericRulerOps to
 * provide a different behavior, e.g. to ignore some values, write some others in
 * different colors, etc.
 *
 * This function is exported so that Subclasses can chain call to it to only change
 * the part they need to alter (e.g. changing the value, the color, ...)
 *
 * @param self a GenericRuler
 * @param value The value to be written
 * @param precision How many digital places to write. 0 for integral part only.
 * @param font The PCF_Font to use
 * @param color The color to use, in the underlying surface format
 * @param x The x position where to write the value
 * @param y The y position where to write the value
 * @param placement The placement (@see PCF_TextPlacement)
 *
 * @return true on success, false otherwise.
 */
bool generic_ruler_etch_tick_label_impl(GenericRuler *self, float value, uint8_t precision, PCF_Font *font, Uint32 color, int x, int y, PCF_TextPlacement placement)
{
    PCF_FontWriteNumberAt(font,
        &value, TypeFloat, precision,
        color, false,
        GENERIC_LAYER(self)->canvas,
        x,y, placement

    );
    return true;
}

/**
 * @brief Given a value, return the pixel increment (relative to the ruler's
 * start) that represents it on the ruler.
 *
 * @param self a GenericRuler
 * @param value The value to translate.
 *
 * @return The corresponding pixel increment, or 0 if the value is -INFINITY.
 */
int generic_ruler_get_pixel_increment_for(GenericRuler *self, float value)
{
    TickMark *base;
    int axis_max;
    int axis_min;
    int rv;

    if(value == -INFINITY) return 0;
    if(value == INFINITY)
        value = GENERIC_STRIP(self)->end;

    base = &self->ticks[0];
    axis_max = generic_ruler_orientation(self, RulerHorizontal)
               ? self->ruler_area.w-1
               : self->ruler_area.h-1;
    axis_min = generic_ruler_orientation(self, RulerHorizontal)
               ? self->ruler_area.x
               : self->ruler_area.y;

    if(fmodf(value, base->value) == 0){ /* are we on a tick mark? */
        float delta = value - self->first_tick.value;
        int nunits = fabsf(delta/base->value);
        if(delta >= 0){
            if(generic_ruler_direction(self, RulerGrowAlongAxis))
                rv = self->first_tick.position + nunits*base->stride;
            else
                rv = self->first_tick.position - nunits*base->stride;
        }else{
            if(generic_ruler_direction(self, RulerGrowAlongAxis))
                rv = self->first_tick.position - nunits*base->stride;
            else
                rv = self->first_tick.position + nunits*base->stride;
        }
        /*we do not need to offset back the returned value from ruler_area
         * because it as been computed from anchor which has not been
         * offseted by ruler_area, i.e anchor is already a relative increment*/
    }else{
        /* we are not on an exact tick mark, so approximate */
        rv = (value - GENERIC_STRIP(self)->start)*(axis_max - 0)/(GENERIC_STRIP(self)->end - GENERIC_STRIP(self)->start) + 0;
        rv = generic_ruler_direction(self, RulerGrowAlongAxis)
             ? rv
             : axis_max - rv;
    }
    return rv;
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
bool generic_ruler_get_size_request(GenericRuler *self, int8_t precision, PCF_Font *font, Location markings_location, int text_margin, int *w, int *h)
{
    int pixel_increment = roundf(self->ticks[0].value * GENERIC_STRIP(self)->ppv);
    int glyph_height = PCF_FontCharHeight(font);

    char vbuffer[10]; /*9999999999 or 999999999 with a floating dot*/
    SDL_Rect m_rect; //mark rect

    /*Pixels to add on each side of the ruler to
     * accomodate etching marks
     * */
    int left, right;
    int top, bottom;

    int fbw = self->ruler_area.w;
    int fbh = self->ruler_area.h;

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

    self->ruler_area = (SDL_Rect){
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
 * @brief Draw the give color zones on the ruler
 *
 * If you want to have a spine line and/or hatch marks on top of said zones
 * you have to draw the zones first, and other etches on top of that.
 *
 * @param self a GenericRuler
 * @param spine_location Location of the spine line of the ruler. Even if you
 * don't etch lines on the ruler at all, this function needs to know from where
 * to start filling the ruler with color, especially if @p fill_ratio is not
 * 1.0. The filling will start on the given location and grow to the opposite
 * e.g. Left->Right, Bottom->Top, etc. The center location will grow as evenly
 * as possible towards both opposite sides.
 * @param nzones Number of zones in the @p zones array
 * @param zones Address to the first of @p nzones zones. If overlapping, the
 * last drawn zone will take precedence.
 * @param fill_ratio amount of the ruler to fill with colors, from 0 (0%)
 * to 1.0 (100%).
 * @return true on success, false otherwise.
 *
 * TODO: When areas overlap, make them stack on top of
 * one another, equally sharing the fill_ratio space.
 */
bool generic_ruler_draw_zones(GenericRuler *self, Location spine_location, int nzones, ColorZone *zones, float fill_ratio)
{
    int begin, end;

    int start_y,end_y;
    int start_x, end_x;
    int npixels;

    if(generic_ruler_orientation(self, RulerHorizontal)){
        /* number of px *to add* and thus can't be self->ruler_area.h
         * which would go overboard as the area goes from area.x to
         * area.x + (area.h-1)
         */
        npixels = (self->ruler_area.h - 1)*fill_ratio;
        if(spine_location == Top){
            start_y = self->ruler_area.y;
            end_y = self->ruler_area.y + npixels;
        }else if(spine_location == Center){
            start_y = SDLExt_RectMidY(&self->ruler_area) - (SDLExt_RectMidY(&self->ruler_area)-self->ruler_area.y)*fill_ratio;
            end_y = SDLExt_RectMidY(&self->ruler_area) + (SDLExt_RectLastY(&self->ruler_area)-SDLExt_RectMidY(&self->ruler_area))*fill_ratio;
        }else if (spine_location == Bottom) {
            start_y = SDLExt_RectLastY(&self->ruler_area) - npixels;
            end_y = SDLExt_RectLastY(&self->ruler_area);
        }else{
            printf("GenericRuler: Unsupported line position for Horizontal orientation");
            return false;
        }
    }else{ /* RulerVertical */
        npixels = (self->ruler_area.w - 1)*fill_ratio;
        if(spine_location == Left){
            start_x = self->ruler_area.x;
            end_x = self->ruler_area.x + npixels;
        }else if(spine_location == Center){
            start_x = SDLExt_RectMidX(&self->ruler_area) - (SDLExt_RectMidX(&self->ruler_area)-self->ruler_area.x)*fill_ratio;
            end_x = SDLExt_RectMidX(&self->ruler_area) + (SDLExt_RectLastX(&self->ruler_area)-SDLExt_RectMidX(&self->ruler_area))*fill_ratio;
        }else if(spine_location == Right){
            start_x = SDLExt_RectLastX(&self->ruler_area) - npixels;
            end_x = SDLExt_RectLastX(&self->ruler_area);
        }else{
            printf("GenericRuler: Unsupported line position for Vertical orientation");
            return false;
        }
    }

    /* Colored areas, if any are assumed to be sorted
     * */
    SDL_LockSurface(GENERIC_LAYER(self)->canvas);
    Uint32 *pixels = GENERIC_LAYER(self)->canvas->pixels;
    for(int i = 0; i < nzones; i++){
        begin = generic_ruler_direction(self, RulerGrowAlongAxis)
                ? generic_ruler_get_pixel_increment_for(self, zones[i].from)
                : generic_ruler_get_pixel_increment_for(self, zones[i].to);
        end = generic_ruler_direction(self, RulerGrowAlongAxis)
              ? generic_ruler_get_pixel_increment_for(self, zones[i].to)
              : generic_ruler_get_pixel_increment_for(self, zones[i].from);
        /* begin/end are the pixels that represent the from/to area bound values
         * if they are excluded, just go one pixel after/before.*/
        if(zones[i].flags & FromExcluded)
            begin += 1;
        if(zones[i].flags & ToExcluded)
            end -= 1;
        Uint32 bcolor = SDL_MapRGBA(GENERIC_LAYER(self)->canvas->format,
            zones[i].color.r,
            zones[i].color.g,
            zones[i].color.b,
            zones[i].color.a
        );
        if(generic_ruler_orientation(self, RulerHorizontal)){
            for(int y = start_y; y <= end_y; y++){
                for(int x = self->ruler_area.x + begin; x <= self->ruler_area.x + end; x++){
                    pixels[y * GENERIC_LAYER(self)->canvas->w + x] = bcolor;
                }
            }
        }else{ /* RulerVertical */
            for(int y = self->ruler_area.y + begin; y <= self->ruler_area.y + end; y++){
                for(int x = start_x; x <= end_x; x++){
                    pixels[y * GENERIC_LAYER(self)->canvas->w + x] = bcolor;
                }
            }
        }
    }
    SDL_UnlockSurface(GENERIC_LAYER(self)->canvas);
    return true;
}

/* private helper functions. Can go public as needed */


/**
 * @brief Helper function to compute the "first" (value-wise) tick mark
 * form the anchor point provided by the user. The anchor point allows to pin
 * the ruler to a certain position, e.g. 0 is in the middle of the ruler.
 * It must be a multiple of the base unit.
 *
 *
 * @param self a GenericRuler
 * @param anchor The anchor point, where a value is mapped to a pixel position.
 *
 */
static void generic_ruler_set_anchor(GenericRuler *self, Anchor anchor)
{
    TickMark *base;
    int axis_min, axis_max;

    base = &(self->ticks[0]);
    axis_min = generic_ruler_axis_min(self);
    axis_max = generic_ruler_axis_max(self);

    if(generic_ruler_direction(self, RulerGrowAlongAxis)){
        int delta = anchor.position - axis_min;
        int nunits = delta/base->stride;
        self->first_tick.position = anchor.position - nunits*base->stride;
        self->first_tick.value = anchor.value - nunits*base->value;
    }else{ /* RulerGrowAgainstAxis */
        int delta = axis_max - anchor.position;
        int nunits = delta/base->stride;
        self->first_tick.position = anchor.position + nunits*base->stride;
        self->first_tick.value = anchor.value - nunits*base->value;
    }
}

/**
 * @brief Helper function to actually draw the hatches(graduations) on the ruler
 * for vertical orientation.
 *
 * @param self a GenericRuler
 * @param color The color to use for hatches, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine. Valid locations for vertical
 * rulers are Left, Center, Right. Ignored when @p spine is false.
 * @param preload_px How much pixels to pretend to have already written.
 *
 * @return true on success, false otherwise.
 */
static bool generic_ruler_draw_hatches_vertical(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location, int preload_px)
{

    SDL_Surface *canvas;
    int spine_x;
    int firstx, lastx;

    canvas = GENERIC_LAYER(self)->canvas;


    if(spine_location == Center)
        spine_x = SDLExt_RectMidX(&self->ruler_area);
    else if(spine_location == Left)
        spine_x = self->ruler_area.x;
    else if(spine_location == Right)
        spine_x = SDLExt_RectLastX(&self->ruler_area);
    else
        return printf("GenericRuler: %d: Unsupported spine position for Vertical orientation\n", spine_location) ? false : false; //TODO: prettyLocation
        //TODO: macro the above trick
    self->spine_coord = spine_x;

    generic_layer_lock(GENERIC_LAYER(self));
    Uint32 *pixels = canvas->pixels;
    Uint32 save_color = color;
    for(int y = SDLExt_RectLastY(&self->ruler_area), cnt = 0; y >= self->ruler_area.y; y--, cnt++){
        firstx = spine_x;
        lastx = firstx;
        int current_px = cnt + preload_px;
        if(current_px % self->ticks[0].stride == 0){ /*tick mark*/
            for(int k = self->nticks-1; k >= 0; k--){
                int size = self->ticks[k].size;
                if(self->ops->will_etch_tick)
                    self->ops->will_etch_tick(self, generic_ruler_position_to_value(self, y), &size, &color);

                if(current_px % self->ticks[k].stride == 0){
                    if(spine_location == Left || spine_location == Center)
                        lastx = min_int(spine_x + size, SDLExt_RectLastX(&self->ruler_area));
                    if(spine_location == Right || spine_location == Center)
                        firstx = max_int(spine_x - size, 0);
                    break;
                }
            }
        }
        if(firstx == lastx && !etch_spine) continue;
        for(int j = firstx; j <= lastx; j++){
#if 0
            if(j > SDLExt_RectLastX(&self->ruler_area) || j < self->ruler_area.x){
                printf("GenericRuler: WARNING j=%d is OOB from (x:%d, w:%d)!\n", j, self->ruler_area.x, self->ruler_area.w);
            }
#endif
            Uint32 idx = y * canvas->w + j;
            pixels[idx] = color;
        }
        color = save_color; /*reset color in case the etch_tick callback changed it*/
    }
    generic_layer_unlock(GENERIC_LAYER(self));
#if DEBUG_GENERIC_RULER
    generic_layer_save_bmp(GENERIC_LAYER(self), "/tmp/ruler-try-vertical.bmp");
#endif
    return true;
}

/**
 * @brief Helper function to actually draw the hatches(graduations) on the ruler
 * for horizontal orientation.
 *
 * @param self a GenericRuler
 * @param color The color to use for hatches, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine. Valid locations for horizontal
 * rulers are Top, Center, Bottom. Ignored when @p spine is false.
 * @param preload_px How much pixels to pretend to have already written.
 *
 * @return true on success, false otherwise.
 */
static bool generic_ruler_draw_hatches_horizontal(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location, int preload_px)
{

    SDL_Surface *canvas;
    TickMark base;
    int spine_y;
    int firstx, lastx;
    int firsty, lasty;

    canvas = GENERIC_LAYER(self)->canvas;
    base = self->ticks[0];

    /*assumes that the biggest mark is the tallest*/
    if(spine_location == Center){
        spine_y = SDLExt_RectMidY(&self->ruler_area);
        firsty  = spine_y - self->ticks[self->nticks-1].size;
        lasty = spine_y + self->ticks[self->nticks-1].size;
    }else if(spine_location == Top){
        spine_y = self->ruler_area.y;
        firsty  = spine_y;
        lasty = spine_y + self->ticks[self->nticks-1].size;
    }else if(spine_location == Bottom){
        spine_y = SDLExt_RectLastY(&self->ruler_area);
        firsty  = spine_y - self->ticks[self->nticks-1].size;
        lasty = spine_y;
    }
    else{
        return printf("GenericRuler: %d: Unsupported spine position for Horizontal orientation\n", spine_location) ? false : false; //TODO: prettyLocation
        //TODO: macro the above trick
    }
    self->spine_coord = spine_y;

    generic_layer_lock(GENERIC_LAYER(self));
    Uint32 *pixels = canvas->pixels;
    for(int y = firsty; y <= lasty; y++){
        for(int x = SDLExt_RectLastX(&self->ruler_area), cnt = 0; x >= self->ruler_area.x; x--, cnt++){
            int current_px = cnt + preload_px;
            if(current_px % base.stride == 0){ /*tick mark*/
                for(int k = self->nticks-1; k >= 0; k--){
                    if(current_px % self->ticks[k].stride == 0){
                        int size = self->ticks[k].size;
                        Uint32 tick_color = color;
                        if(self->ops->will_etch_tick){
                            self->ops->will_etch_tick(self, generic_ruler_position_to_value(self, x), &size, &tick_color);
                        }
                        if(size && size >= abs(y - spine_y)){
                            pixels[y * canvas->w + x] = tick_color;
                        }
                        break;
                    }
                }
            }
            if(etch_spine && y == spine_y){
                pixels[y * canvas->w + x] = color;
            }
        }
    }

    generic_layer_unlock(GENERIC_LAYER(self));
#if DEBUG_GENERIC_RULER
    generic_layer_save_bmp(GENERIC_LAYER(self), "/tmp/ruler-try-horizontal.bmp");
#endif
    return true;
}

/**
 * @brief Helper function to actually draw the markings (numbers) on the ruler
 * for vertical orientation.
 *
 * @param self a GenericRuler
 * @param location Where to put the markings. Valid locations for vertical rulers
 * are: Left and Right.
 * @param text_margin How many pixels to leave between the ruler and the markings.
 * When unsure, use GENERIC_RULER_DEFAULT_TEXT_MARGIN.
 * @param font The PCF_Font to use.
 * @param color The color to use, in the underlying surface format
 * @param precision How many digital places to write. 0 for integral part only.
 *
 * @return true on success, false otherwise.
 */
static bool generic_ruler_etch_markings_vertical(GenericRuler *self, Location markings_location, int text_margin, PCF_Font *font, Uint32 color, uint8_t precision)
{
    int start_pos;
    float start_value;
    TickMark *base;

    base = &self->ticks[0];

    start_pos = self->first_tick.position;
    start_value = self->first_tick.value;

    float value;
    int y;
    for(y = self->ruler_area.y + start_pos, value = start_value;
        generic_ruler_direction(self, RulerGrowAlongAxis) ? y <= SDLExt_RectLastY(&(self->ruler_area)) : y >= self->ruler_area.y;
        y += generic_ruler_direction(self, RulerGrowAlongAxis) ? base->stride : -base->stride, value += base->value){

        TickMark *current_tick;
        for(int k = self->nticks-1; k >= 0; k--){
            current_tick = &self->ticks[k];
            if(fmodf(value, current_tick->value) == 0){
                break;
            }
        }
        if(current_tick->labeled == false) continue;

        if(markings_location & Left){
            generic_ruler_etch_tick_label(self, value, precision, font, color,
                self->spine_coord - current_tick->size - text_margin,
                y,
                LeftToCol | CenterOnRow
            );
        }
        if(markings_location & Right){
            generic_ruler_etch_tick_label(self, value, precision, font, color,
                self->spine_coord + current_tick->size + text_margin,
                y,
                RightToCol | CenterOnRow
            );
        }
    }
#if DEBUG_GENERIC_RULER
    generic_layer_save_bmp(GENERIC_LAYER(self), "/tmp/ruler-try-hatches.bmp");
#endif
    return true;
}

/**
 * @brief Helper function to actually draw the markings (numbers) on the ruler
 * for horizontal orientation.
 *
 * @param self a GenericRuler
 * @param location Where to put the markings. Valid locations for horizontal rulers
 * are: Top and Bottom.
 * @param text_margin How many pixels to leave between the ruler and the markings.
 * When unsure, use GENERIC_RULER_DEFAULT_TEXT_MARGIN.
 * @param font The PCF_Font to use.
 * @param color The color to use, in the underlying surface format
 * @param precision How many digital places to write. 0 for integral part only.
 *
 * @return true on success, false otherwise.
 */
static bool generic_ruler_etch_markings_horizontal(GenericRuler *self, Location markings_location, int text_margin, PCF_Font *font, Uint32 color, uint8_t precision)
{
    int start_pos;
    float start_value;
    TickMark *base;

    base = &self->ticks[0];

    start_pos = self->first_tick.position;
    start_value = self->first_tick.value;

    for(int x = self->ruler_area.x + start_pos, value = start_value;
        generic_ruler_direction(self, RulerGrowAlongAxis) ? x <= SDLExt_RectLastX(&(self->ruler_area)) : x >= self->ruler_area.x;
        x += generic_ruler_direction(self, RulerGrowAlongAxis) ? base->stride : -base->stride, value += base->value){

        TickMark *current_tick;
        for(int k = self->nticks-1; k >= 0; k--){
            current_tick = &self->ticks[k];
            if(fmodf(value, current_tick->value) == 0){
                break;
            }
        }
        if(current_tick->labeled == false) continue;

        if(markings_location & Top){
            generic_ruler_etch_tick_label(self, value, precision, font, color,
                x,
                self->spine_coord - current_tick->size - text_margin,
                CenterOnCol | AboveRow
            );
        }
        if(markings_location & Bottom){
            generic_ruler_etch_tick_label(self, value, precision, font, color,
                x,
                self->spine_coord + current_tick->size + text_margin,
                CenterOnCol | BelowRow
            );
        }
    }

    return true;
}
