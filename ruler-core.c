#include <stdio.h>
#include <stdlib.h>

#include "SDL_pcf.h"
#include "ruler-core.h"
#include "generic-strip.h"

/**
 * RulerCore works together with a GenericStrip to provide reusable features for
 * handling cases where:
 * a) a range of values is mapped to a pixel range
 * b) that range of values have one or more level of graduations (tick marks)
 * at regular intervals.
 *
 * Users of RulerCore are responsible for the sequence of initing the object and
 * using the provided functions to draw what's needed for their use case like
 * ticks and labels.
 *
 * RulerCore provides the features needed to resolve a value to a pixel
 * position and vice-versa.
 *
 * Callers are responsible for the sequence of initing the object and using the
 * provided functions to draw what's needed for their use case like ticks,
 * labels, color zones, etc.
 *
 * RulerCore can use two coordinates systems: The canvas or image coordinates
 * that goes from 0 to the canvas width/height and the ruler coordinates
 * that goes from 0 to the ruler width/height. The ruler coordinates are used
 * when we need a ruler that is smaller than the canvas to we have enough room
 * to write the labels around it.
 *
 * RulerCore uses an anchor point to map a value (that must be a multiple of
 * the base unit) to a pixel position (ruler coordinates).
 * The anchor point is defined by a value and a pixel position : Let's say that
 * the value "0" is in the middle of the ruler, which will be for a 101px ruler
 * at pixel 50. Given that, the RulerCore will compute the first tick
 * position (leftmost for a horizontal GrowAlongAxis ruler) and store it.
 */

/*private forward declarations*/
static bool ruler_core_get_zone_extent(RulerCore *self, GenericStrip *strip,
                                       ColorZone *zone, Location spine_location,
                                       float fill_ratio, int *start, int *end);
static bool ruler_core_rasterize_ticks(RulerCore *self, GenericStrip *strip,
                                       Uint32 color, bool etch_spine,
                                       Location spine_location, int preload_px,
                                       WillDrawTickFunc will_draw_tick);
static bool ruler_core_rasterize_ticks_vertical(RulerCore *self, GenericStrip *strip,
                                                Uint32 color, bool etch_spine,
                                                Location spine_location, int preload_px,
                                                WillDrawTickFunc will_draw_tick);
static bool ruler_core_rasterize_ticks_horizontal(RulerCore *self, GenericStrip *strip,
                                                  Uint32 color, bool etch_spine,
                                                  Location spine_location, int preload_px,
                                                  WillDrawTickFunc will_draw_tick);
static bool ruler_core_rasterize_labels_vertical(RulerCore *self, GenericStrip *strip,
                                                 Location labels_location, int text_margin,
                                                 PCF_Font *font, Uint32 color, uint8_t precision,
                                                 DrawTickLabelFunc draw_tick_label);
static bool ruler_core_rasterize_labels_horizontal(RulerCore *self, GenericStrip *strip,
                                                   Location labels_location, int text_margin,
                                                   PCF_Font *font, Uint32 color, uint8_t precision,
                                                   DrawTickLabelFunc draw_tick_label);
static int ruler_core_color_zone_location_cmp(const void *a, const void *b);
static int ruler_core_color_zone_divider_cmp(const void *a, const void *b);


/**
 * @brief Helper function for init functions. Turns a a user-friendly description
 * of the ticks to a more convenient internal representation.
 *
 * @param self a RulerCore
 * @param base_unit The value of the base unit, e.g. 5.
 * @param base_unit_px The number of pixels that represent the base unit,
 * e.g. 11px.
 * @param base_unit_size The size (over the spine) of the base unit tick
 * mark in pixels, e.g. 5px.
 * @param base_unit_labeled Whether the base unit tick mark will get a label.
 * @param nmultiples The size of the following consecutives RulerUnitMultiples,
 * 0 when none.
 * @param multiples Address of the first RulerUnitMultiple in a series of
 * @p nmultiples that will be used to represent how multiples of the base unit
 * will be drawn. The first element of the array will be used to represent the
 * first multiple of the base unit (@p base_unit), the second element will be
 * used to represent the second multiple of the base unit, and so on. NULL when
 * no multiples are to be used.
 *
 */
bool ruler_core_set_ticks(RulerCore *self, float base_unit, uint16_t base_unit_px,
                          uint8_t base_unit_size, bool base_unit_labeled,
                          uint8_t nmultiples, RulerUnitMultiple *multiples)

{
    if(nmultiples > RULER_CORE_MAX_TICKS)
        return false;

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

    return true;
}

/**
 * @brief Helper function to compute the "first" (value-wise) tick mark
 * form the anchor point provided by the user. The anchor point allows to pin
 * the ruler to a certain position, e.g. 0 is in the middle of the ruler.
 * It must be a multiple of the base unit.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param anchor The anchor point, where a value is mapped to a pixel position.
 *
 */
void ruler_core_set_anchor(RulerCore *self, GenericStrip *strip, Anchor anchor)
{
    TickMark *base;
    int axis_min, axis_max;

    base = &(self->ticks[0]);
    axis_min = ruler_core_axis_min(self, strip);
    axis_max = ruler_core_axis_max(self, strip);

    if(generic_strip_direction(strip, GROW_ALONG_AXIS)){
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

int ruler_core_get_spine_coord(RulerCore *self, RulerOrientation orientation,
                               Location spine_location)
{
    int rv = -1;
    if(orientation == RulerVertical){
        if(spine_location == Center)
            rv = SDLExt_RectMidX(&self->ruler_area);
        else if(spine_location == Left)
            rv = self->ruler_area.x;
        else if(spine_location == Right)
            rv = SDLExt_RectLastX(&self->ruler_area);
    }else{ //RulerHorizontal
        if(spine_location == Center)
            rv = SDLExt_RectMidY(&self->ruler_area);
        else if(spine_location == Top)
            rv = self->ruler_area.y;
        else if(spine_location == Bottom)
            rv = SDLExt_RectLastY(&self->ruler_area);
    }
    return rv;
}

/**
 * @brief Convert a pixel position within the @p strip's canvas that obeys
 * RulerCore to a value within the ruler's range. This function accepts a
 * pixel position from 0 to the canvas width/height (depending on the
 * orientation) and returns the corresponding value within the ruler's range.
 * If the pixel position is outside the actual ruler_rect portion of the canvas,
 * the function will return NAN.
 *
 * Contrary to ruler_core_increment_to_value, this function accepts a pixel
 * absolute position within the canvas, not a pixel increment relative to the
 * ruler's start.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param px_position The pixel position to convert.
 *
 * @return The corresponding value within the ruler's range, or NAN if the pixel
 * position is on the canvas but outside the ruler.
 *
 * @see ruler_core_increment_to_value
 */
float ruler_core_position_to_value(RulerCore *self, GenericStrip *strip, unsigned int px_position)
{
    int axis_min, axis_max;

    axis_min = ruler_core_axis_min(self, strip);
    axis_max = ruler_core_axis_max(self, strip);

    if(px_position < axis_min || px_position > axis_max){
        printf("%s: px_position(%d) is out of bounds (min:%d, max:%d)\n", __FUNCTION__, px_position, axis_min, axis_max);
        return NAN;
    }

    return ruler_core_increment_to_value(self, strip, px_position - axis_min);
}

/**
 * @brief Convert a pixel increment (relative to the ruler's start) to a value
 * within the ruler's range.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param px_increment The pixel increment to convert.
 *
 * @return The corresponding value within the ruler's range, or NAN if the pixel
 * increment is out of bounds.
 *
 * @see ruler_core_position_to_value
 */
float ruler_core_increment_to_value(RulerCore *self, GenericStrip *strip, unsigned int px_increment)
{
    int delta;
    int axis_max;
    TickMark *base;

    base = &(self->ticks[0]);
    /* we don't use ruler_core_axis_max because we need the value in ruler
     * coordinates, not image coordinates. */
    axis_max = generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
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
            if(generic_strip_direction(strip, GROW_ALONG_AXIS))
                return self->first_tick.value + nunits*base->value;
            else
                return self->first_tick.value - nunits*base->value;
        }else{
            if(generic_strip_direction(strip, GROW_ALONG_AXIS))
                return self->first_tick.value - nunits*base->value;
            else
                return self->first_tick.value + nunits*base->value;
        }
    }
    /*otherwise we resort to approximation*/
    if(generic_strip_direction(strip, GROW_ALONG_AXIS))
        return strip->start + px_increment/strip->ppv;
    else
        return strip->start + (axis_max - px_increment)/strip->ppv;
}

/**
 * @brief Write ruler ticks(graduations) on @p strip
 *
 * Drawing can be customized by providing a callback function that will be
 * called before drawing each tick mark. It can be used to change the size or
 * color of the tick mark.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param color The color to use for ticks, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine. Valid locations for
 * Horizontal gauges are Top,Center,Bottom and Left, Center, Right for Vertical
 * gauges. Ignored when @p spine is false.
 * @param will_draw_tick A callback function that will be called before drawing
 * each tick mark. It can be used to change the size or color of the tick mark.
 * set to NULL to use the default behavior.
 *
 * @return true on success, false otherwise.
 */
bool ruler_core_draw_ticks(RulerCore *self, GenericStrip *strip,
                           Uint32 color, bool etch_spine, Location spine_location,
                           WillDrawTickFunc will_draw_tick)
{
    TickMark base;
    Anchor anchor;
    int axis_max;


    base = self->ticks[0];
    anchor = self->first_tick;
    /* we don't use ruler_core_axis_max because we need the value in ruler
     * coordinates, not image coordinates. */
    axis_max = generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
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
        anchor.position += generic_strip_direction(strip, GROW_ALONG_AXIS)
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
    return ruler_core_rasterize_ticks(
        self, strip, color, etch_spine,
        spine_location, preload_px, will_draw_tick
    );
}

/**
 * @brief Draw labels (numbers) on the ruler at the predefined
 * step
 *
 * The actual drawing of the labels is done by calling the @p draw_tick_label
 * callback function. This allows callers to customize the way the labels are
 * drawn. The default implemntation of the callback function is provided by the
 * ruler_core_draw_tick_label_impl function, which just writes the value at the
 * given position using the appropriate font and color. This function will be
 * called if @p draw_tick_label is NULL, and is exported so that callers can
 * chain call to it to only change the part they need to alter (e.g. changing
 * the value, the color, ...).
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param location Where to put the labels. Valid locations for Horizontal
 * rulers are: Top, Bottom while valid locations for Vertical gauges are
 * Left and Right.
 * @param text_margin How many pixels to leave between the ruler and the labels.
 * When unsure, use RULER_CORE_DEFAULT_TEXT_MARGIN.
 * @param font The PCF_Font to use.
 * @param color The color to use, in the underlying surface format
 * @param precision How many digital places to write. 0 for integral part only.
 * @param draw_tick_label A callback function that will be called to actually
 * draw the tick label. It can be used to change the way the label is drawn.
 * Set to NULL to use the default behavior.
 *
 * @return true on success, false otherwise.
 */
bool ruler_core_draw_labels(RulerCore *self, GenericStrip *strip,
                            Location labels_location, int text_margin,
                            PCF_Font *font, Uint32 color, uint8_t precision,
                            DrawTickLabelFunc draw_tick_label)
{
    draw_tick_label = draw_tick_label
                    ? draw_tick_label
                    : ruler_core_draw_tick_label_impl;

    return generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
           ? ruler_core_rasterize_labels_horizontal(self, strip, labels_location,
                                                    text_margin, font, color,
                                                    precision, draw_tick_label)
           : ruler_core_rasterize_labels_vertical(self, strip, labels_location,
                                                  text_margin, font, color,
                                                  precision, draw_tick_label);
#if DEBUG_RULER_CORE
    generic_layer_save_bmp(strip, "/tmp/ruler-try-ticks.bmp");
#endif
}

/**
 * @brief This is the default implementation of the draw_tick_label operation,
 * that just writes the value at the given position using the appropriate font
 * and color.
 *
 * Subclasses can override this operation to provide a different behavior,
 * e.g. to ignore some values, write some others in different colors, etc.
 *
 * This function is exported so that Subclasses can chain call to it to only change
 * the part they need to alter (e.g. changing the value, the color, ...)
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
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
bool ruler_core_draw_tick_label_impl(RulerCore *self, GenericStrip *strip,
                                     float value, uint8_t precision,
                                     PCF_Font *font, Uint32 color, int x,
                                     int y, PCF_TextPlacement placement)
{
    PCF_FontWriteNumberAt(font,
        &value, TypeFloat, precision,
        color, false,
        GENERIC_LAYER(strip)->canvas,
        x,y, placement

    );
    return true;
}

/**
 * @brief Given a value, return the pixel increment (relative to the ruler's
 * start coordinate) that represents it on the ruler.
 *
 * Warning: This function does not check if the value is within the range of
 * the ruler. If the value is outside the ruler's range, it will return a
 * mathematically correct pixel increment that will be outside of the canvas
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param value The value to translate, assumed to be between
 * @p strip->begin and @p strip->end.
 *
 * @return The corresponding pixel increment.
 */
int ruler_core_get_pixel_increment_for(RulerCore *self, GenericStrip *strip, float value)
{
    TickMark *base;
    int axis_max;
    int axis_min;
    int rv;

    /*TODO: Double check if that is used and where*/
    if(value == -INFINITY) return 0;
    if(value == INFINITY)
        value = strip->end;
    /*TODO: Do we need to clamp the value?*/

    base = &self->ticks[0];
    /* we don't use ruler_core_axis_max because we need the value in ruler
     * coordinates, not image coordinates. */
    axis_max = generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
               ? self->ruler_area.w-1
               : self->ruler_area.h-1;
    axis_min = generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
               ? self->ruler_area.x
               : self->ruler_area.y;

    if(fmodf(value, base->value) == 0){ /* are we on a tick mark? */
        float delta = value - self->first_tick.value;
        int nunits = fabsf(delta/base->value);
        if(delta >= 0){
            if(generic_strip_direction(strip, GROW_ALONG_AXIS))
                rv = self->first_tick.position + nunits*base->stride;
            else
                rv = self->first_tick.position - nunits*base->stride;
        }else{
            if(generic_strip_direction(strip, GROW_ALONG_AXIS))
                rv = self->first_tick.position - nunits*base->stride;
            else
                rv = self->first_tick.position + nunits*base->stride;
        }
        /*we do not need to offset back the returned value from ruler_area
         * because it as been computed from anchor which has not been
         * offseted by ruler_area, i.e anchor is already a relative increment*/
    }else{
        /* we are not on an exact tick mark, so approximate */
        rv = (value - strip->start)*(axis_max - 0)/(strip->end - strip->start) + 0;
        rv = generic_strip_direction(strip, GROW_ALONG_AXIS)
             ? rv
             : axis_max - rv;
    }
    return rv;
}

/**
 * @brief Preprocess the ruler's color zones so they can be used by
 * ruler_core_draw_zones. Calling this function is mandatory.
 *
 * This function detects overlapping zones and adjusts their properties so that
 * they can be drawn correctly.
 *
 * @param nzones Number of zones in the @p zones array
 * @param zones Address to the first of @p nzones zones.
 *
 * @see ruler_core_draw_zones
 */
void ruler_core_preprocess_color_zones(int nzones, ColorZone *zones)
{
    qsort(zones, nzones, sizeof(ColorZone), ruler_core_color_zone_location_cmp);
    for(int i = 0; i < nzones; i++){
        zones[i].divider = 1;
        for(int k = i+1; k < nzones; k++){
            //We use a strict comparison to avoid having a border count as an overlap.
            if(zones[k].from > zones[i].from && zones[k].from < zones[i].to){
                zones[i].divider++;
            }
        }
    }
    qsort(zones, nzones, sizeof(ColorZone), ruler_core_color_zone_divider_cmp);
}

/**
 * @brief Draw the give color zones on the ruler
 *
 * If you want to have a spine line and/or hatch marks on top of said zones
 * you have to draw the zones first, and other draws on top of that.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param spine_location Location of the spine line of the ruler. Even if you
 * don't draw lines on the ruler at all, this function needs to know from where
 * to start filling the ruler with color, especially if @p fill_ratio is not
 * 1.0. The filling will start on the given location and grow to the opposite
 * e.g. Left->Right, Bottom->Top, etc. The center location will grow as evenly
 * as possible towards both opposite sides.
 * @param nzones Number of zones in the @p zones array
 * @param zones Address to the first of @p nzones zones. These zones *must* have
 * been preprocessed by ruler_core_preprocess_color_zones.
 * @param fill_ratio amount of the ruler to fill with colors, from 0 (0%)
 * to 1.0 (100%). 100% is as high as the longest tick mark, 0% is no fill at all.
 * @return true on success, false otherwise.
 *
 * @see ruler_core_preprocess_color_zones
 */
bool ruler_core_draw_zones(RulerCore *self, GenericStrip *strip,
                           Location spine_location, int nzones, ColorZone *zones,
                           float fill_ratio)
{
    int begin, end;

    int start_y,end_y;
    int start_x, end_x;
    int npixels;

    self->spine_coord = ruler_core_get_spine_coord(
        self,
        generic_strip_orientation(strip, ORIENTATION_HORIZONTAL) ? RulerHorizontal : RulerVertical,
        spine_location
    );

    /* Colored areas, if any are assumed to be sorted
     * */
    SDL_LockSurface(GENERIC_LAYER(strip)->canvas);
    Uint32 *pixels = GENERIC_LAYER(strip)->canvas->pixels;
    for(int i = 0; i < nzones; i++){
        float vbegin, vend;
        bool intersected = interval_intersect(
            zones[i].from, zones[i].to,
            strip->start, strip->end,
        &vbegin, &vend);
        if(!intersected) continue;

        begin = generic_strip_direction(strip, GROW_ALONG_AXIS)
                ? ruler_core_get_pixel_increment_for(self, strip, vbegin)
                : ruler_core_get_pixel_increment_for(self, strip, vend);
        end = generic_strip_direction(strip, GROW_ALONG_AXIS)
              ? ruler_core_get_pixel_increment_for(self, strip, vend)
              : ruler_core_get_pixel_increment_for(self, strip, vbegin);
        /* begin/end are the pixels that represent the from/to area bound values
         * if they are excluded, just go one pixel after/before.*/
        if(zones[i].flags & FromExcluded)
            begin += 1;
        if(zones[i].flags & ToExcluded)
            end -= 1;
        Uint32 bcolor = SDL_MapRGBA(GENERIC_LAYER(strip)->canvas->format,
            zones[i].color.r,
            zones[i].color.g,
            zones[i].color.b,
            zones[i].color.a
        );
        if(generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)){
            ruler_core_get_zone_extent(self, strip, &zones[i], spine_location, fill_ratio, &start_y, &end_y);
            for(int y = start_y; y <= end_y; y++){
                for(int x = self->ruler_area.x + begin; x <= self->ruler_area.x + end; x++){
                    pixels[y * GENERIC_LAYER(strip)->canvas->w + x] = bcolor;
                }
            }
        }else{ /* RulerVertical */
            ruler_core_get_zone_extent(self, strip, &zones[i], spine_location, fill_ratio, &start_x, &end_x);
            for(int y = self->ruler_area.y + begin; y <= self->ruler_area.y + end; y++){
                for(int x = start_x; x <= end_x; x++){
                    pixels[y * GENERIC_LAYER(strip)->canvas->w + x] = bcolor;
                }
            }
        }
    }
    SDL_UnlockSurface(GENERIC_LAYER(strip)->canvas);
    return true;
}


/* private helper functions. Can go public as needed */

/**
 * @brief Helper function for ruler_core_draw_zones to turn the fill ratio
 * into a pixel extent for a given zone. This is the size of the zone "accros"
 * the ruler axis (height of the zone for an horizontal ruler and its width for
 * a vertical one), not to be confused with the length of the zone along the
 * ruler's axis.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param spine_location Where the spine (drawn or not) is locatre. Valid
 * locations for horizontal rulers are Top, Center, Bottom and for vertical
 * rulers Left, Center, Right.
 * @param fill_ratio amount of the ruler to fill with colors, from 0 (0%)
 * to 1.0 (100%). 100% is as high as the longest tick mark, 0% is no fill at all.
 * @param zone The zone to compute the extent for.
 * @param start Where to put the computed start pixel coordinate of the zone extent.
 * @param end Where to put the computed end pixel coordinate of the zone extent.
 *
 * @return true on success, false otherwise.
 *
 * @see ruler_core_draw_zones
 */
static bool ruler_core_get_zone_extent(RulerCore *self, GenericStrip *strip,
                                       ColorZone *zone, Location spine_location,
                                       float fill_ratio, int *start, int *end)
{
    int npixels;
    if(generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)){
        /* number of px *to add* and thus can't be tick.size
         * which would go overboard as the area goes from area.x to
         * area.x + (tick.size-1)
         */
        npixels = (self->ticks[self->nticks-1].size)*fill_ratio; //assumes higher value tick is the longest
        npixels /= zone->divider;
        if(spine_location == Top){
            *start = self->ruler_area.y;
            *end = self->ruler_area.y + npixels;
        }else if(spine_location == Center){
            *start = self->spine_coord - npixels;
            *end = self->spine_coord + npixels;
        }else if (spine_location == Bottom) {
            *start = SDLExt_RectLastY(&self->ruler_area) - npixels;
            *end = SDLExt_RectLastY(&self->ruler_area);
        }else{
            printf("RulerCore: Unsupported line position for Horizontal orientation\n");
            return false;
        }
    }else{ /* RulerVertical */
        npixels = (self->ticks[self->nticks-1].size)*fill_ratio;
        npixels /= zone->divider;
        if(spine_location == Left){
            *start = self->ruler_area.x;
            *end = self->ruler_area.x + npixels;
        }else if(spine_location == Center){
            *start = self->spine_coord - npixels;
            *end = self->spine_coord + npixels;
        }else if(spine_location == Right){
            *start = SDLExt_RectLastX(&self->ruler_area) - npixels;
            *end = SDLExt_RectLastX(&self->ruler_area);
        }else{
            printf("RulerCore: Unsupported line position for Vertical orientation\n");
            return false;
        }
    }
    return true;
}

/**
 * @brief Draw the ticks(graduations) on the ruler.
 *
 * This function does a pure graphical draw of the ticks, it does not consider
 * values. From pixel 0 to the end of the ruler, it will draw ticks at each
 * multiple of the TickMarks of @p self. @p preload_px allows to pretend that
 * some pixels have already been drawn, so that the ticks will be drawn as if
 * the ruler was already drawn up to that. See ruler_core_draw_ticks for
 * an example of how to use this.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param color The color to use for ticks, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine.
 * @param preload_px How much pixels to pretend to have already written.
 * @param will_draw_tick A callback function that will be called before drawing
 * each tick mark. It can be used to change the size or color of the tick mark.
 * Pass NULL to use the default behavior.
 *
 *
 * @return true on success, false otherwise.
 */
static bool ruler_core_rasterize_ticks(RulerCore *self, GenericStrip *strip,
                                       Uint32 color, bool etch_spine,
                                       Location spine_location, int preload_px,
                                       WillDrawTickFunc will_draw_tick)
{
    return generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
           ? ruler_core_rasterize_ticks_horizontal(self, strip, color, etch_spine,
                                                spine_location, preload_px,
                                                will_draw_tick)
           : ruler_core_rasterize_ticks_vertical(self, strip, color, etch_spine,
                                                 spine_location, preload_px,
                                                 will_draw_tick);
}

/**
 * @brief Helper function to actually draw the ticks(graduations) on the ruler
 * for vertical orientation.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param color The color to use for ticks, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine. Valid locations for vertical
 * rulers are Left, Center, Right. Ignored when @p spine is false.
 * @param preload_px How much pixels to pretend to have already written.
 * @param will_draw_tick A callback function that will be called before drawing
 * each tick mark. It can be used to change the size or color of the tick mark.
 * Pass NULL to use the default behavior.
 *
 * @return true on success, false otherwise.
 */
static bool ruler_core_rasterize_ticks_vertical(RulerCore *self, GenericStrip *strip,
                                                Uint32 color, bool etch_spine,
                                                Location spine_location, int preload_px,
                                                WillDrawTickFunc will_draw_tick)
{

    SDL_Surface *canvas;
    int spine_x;
    int firstx, lastx;

    canvas = GENERIC_LAYER(strip)->canvas;


    if(spine_location == Center)
        spine_x = SDLExt_RectMidX(&self->ruler_area);
    else if(spine_location == Left)
        spine_x = self->ruler_area.x;
    else if(spine_location == Right)
        spine_x = SDLExt_RectLastX(&self->ruler_area);
    else
        return printf("RulerCore: %d: Unsupported spine position for Vertical orientation\n", spine_location) ? false : false; //TODO: prettyLocation
        //TODO: macro the above trick
    self->spine_coord = spine_x;

    generic_layer_lock(GENERIC_LAYER(strip));
    Uint32 *pixels = canvas->pixels;
    Uint32 save_color = color;
    for(int y = SDLExt_RectLastY(&self->ruler_area), cnt = 0; y >= self->ruler_area.y; y--, cnt++){
        firstx = spine_x;
        lastx = firstx;
        int current_px = cnt + preload_px;
        if(current_px % self->ticks[0].stride == 0){ /*tick mark*/
            for(int k = self->nticks-1; k >= 0; k--){
                int size = self->ticks[k].size;
                if(will_draw_tick)
                   will_draw_tick(self, strip, ruler_core_position_to_value(self, strip, y), &size, &color);

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
                printf("RulerCore: WARNING j=%d is OOB from (x:%d, w:%d)!\n", j, self->ruler_area.x, self->ruler_area.w);
            }
#endif
            Uint32 idx = y * canvas->w + j;
            pixels[idx] = color;
        }
        color = save_color; /*reset color in case the draw_tick callback changed it*/
    }
    generic_layer_unlock(GENERIC_LAYER(strip));
#if DEBUG_RULER_CORE
    generic_layer_save_bmp(GENERIC_LAYER(strip), "/tmp/ruler-try-vertical.bmp");
#endif
    return true;
}

/**
 * @brief Helper function to actually draw the ticks(graduations) on the ruler
 * for horizontal orientation.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param color The color to use for ticks, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param spine_location Where to draw the spine. Valid locations for horizontal
 * rulers are Top, Center, Bottom. Ignored when @p spine is false.
 * @param preload_px How much pixels to pretend to have already written.
 * @param will_draw_tick A callback function that will be called before drawing
 * each tick mark. It can be used to change the size or color of the tick mark.
 * Pass NULL to use the default behavior.
 *
 * @return true on success, false otherwise.
 */
static bool ruler_core_rasterize_ticks_horizontal(RulerCore *self, GenericStrip *strip,
                                                  Uint32 color, bool etch_spine,
                                                  Location spine_location, int preload_px,
                                                  WillDrawTickFunc will_draw_tick)
{

    SDL_Surface *canvas;
    TickMark base;
    int spine_y;
    int firstx, lastx;
    int firsty, lasty;

    canvas = GENERIC_LAYER(strip)->canvas;
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
        return printf("RulerCore: %d: Unsupported spine position for Horizontal orientation\n", spine_location) ? false : false; //TODO: prettyLocation
        //TODO: macro the above trick
    }
    self->spine_coord = spine_y;

    generic_layer_lock(GENERIC_LAYER(strip));
    Uint32 *pixels = canvas->pixels;
    for(int y = firsty; y <= lasty; y++){
        for(int x = SDLExt_RectLastX(&self->ruler_area), cnt = 0; x >= self->ruler_area.x; x--, cnt++){
            int current_px = cnt + preload_px;
            if(current_px % base.stride == 0){ /*tick mark*/
                for(int k = self->nticks-1; k >= 0; k--){
                    if(current_px % self->ticks[k].stride == 0){
                        int size = self->ticks[k].size;
                        Uint32 tick_color = color;
                        if(will_draw_tick){
                           will_draw_tick(self, strip, ruler_core_position_to_value(self, strip, x), &size, &tick_color);
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

    generic_layer_unlock(GENERIC_LAYER(strip));
#if DEBUG_RULER_CORE
    generic_layer_save_bmp(GENERIC_LAYER(strip), "/tmp/ruler-try-horizontal.bmp");
#endif
    return true;
}

/**
 * @brief Helper function to actually draw the labels (numbers) on the ruler
 * for vertical orientation.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param location Where to put the labels. Valid locations for vertical rulers
 * are: Left and Right.
 * @param text_margin How many pixels to leave between the ruler and the labels.
 * When unsure, use GENERIC_RULER_DEFAULT_TEXT_MARGIN.
 * @param font The PCF_Font to use.
 * @param color The color to use, in the underlying surface format
 * @param precision How many digital places to write. 0 for integral part only.
 * @param draw_tick_label A callback function that will be called to actually
 * draw the tick label. Cannot be NULL. Pass ruler_core_draw_tick_label_impl
 * to use the default behavior.
 *
 * @return true on success, false otherwise.
 */
static bool ruler_core_rasterize_labels_vertical(RulerCore *self, GenericStrip *strip,
                                                 Location labels_location, int text_margin,
                                                 PCF_Font *font, Uint32 color, uint8_t precision,
                                                 DrawTickLabelFunc draw_tick_label)
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
        generic_strip_direction(strip, GROW_ALONG_AXIS) ? y <= SDLExt_RectLastY(&(self->ruler_area)) : y >= self->ruler_area.y;
        y += generic_strip_direction(strip, GROW_ALONG_AXIS) ? base->stride : -base->stride, value += base->value){

        TickMark *current_tick;
        for(int k = self->nticks-1; k >= 0; k--){
            current_tick = &self->ticks[k];
            if(fmodf(value, current_tick->value) == 0){
                break;
            }
        }
        if(current_tick->labeled == false) continue;

        if(labels_location & Left){
            draw_tick_label(self, strip, value, precision, font, color,
                self->spine_coord - current_tick->size - text_margin,
                y,
                LeftToCol | CenterOnRow
            );
        }
        if(labels_location & Right){
            draw_tick_label(self, strip, value, precision, font, color,
                self->spine_coord + current_tick->size + text_margin,
                y,
                RightToCol | CenterOnRow
            );
        }
    }
#if DEBUG_RULER_CORE
    generic_layer_save_bmp(GENERIC_LAYER(strip), "/tmp/ruler-try-ticks.bmp");
#endif
    return true;
}

/**
 * @brief Helper function to actually draw the labels (numbers) on the ruler
 * for horizontal orientation.
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 * @param location Where to put the labels. Valid locations for horizontal rulers
 * are: Top and Bottom.
 * @param text_margin How many pixels to leave between the ruler and the labels.
 * When unsure, use RULER_CORE_DEFAULT_TEXT_MARGIN.
 * @param font The PCF_Font to use.
 * @param color The color to use, in the underlying surface format
 * @param precision How many digital places to write. 0 for integral part only.
 * @param draw_tick_label A callback function that will be called to actually
 * draw the tick label. Cannot be NULL. Pass ruler_core_draw_tick_label_impl
 * to use the default behavior.
 *
 * @return true on success, false otherwise.
 */
static bool ruler_core_rasterize_labels_horizontal(RulerCore *self, GenericStrip *strip,
                                                   Location labels_location, int text_margin,
                                                   PCF_Font *font, Uint32 color, uint8_t precision,
                                                   DrawTickLabelFunc draw_tick_label)
{
    int start_pos;
    float start_value;
    TickMark *base;

    base = &self->ticks[0];

    start_pos = self->first_tick.position;
    start_value = self->first_tick.value;

    for(int x = self->ruler_area.x + start_pos, value = start_value;
        generic_strip_direction(strip, GROW_ALONG_AXIS) ? x <= SDLExt_RectLastX(&(self->ruler_area)) : x >= self->ruler_area.x;
        x += generic_strip_direction(strip, GROW_ALONG_AXIS) ? base->stride : -base->stride, value += base->value){

        TickMark *current_tick;
        for(int k = self->nticks-1; k >= 0; k--){
            current_tick = &self->ticks[k];
            if(fmodf(value, current_tick->value) == 0){
                break;
            }
        }
        if(current_tick->labeled == false) continue;

        if(labels_location & Top){
            draw_tick_label(self, strip, value, precision, font, color,
                x,
                self->spine_coord - current_tick->size - text_margin,
                CenterOnCol | AboveRow
            );
        }
        if(labels_location & Bottom){
            draw_tick_label(self, strip, value, precision, font, color,
                x,
                self->spine_coord + current_tick->size + text_margin,
                CenterOnCol | BelowRow
            );
        }
    }

    return true;
}

/**
 * @brief Helper function to sort ColorZones by their 'from' value.
 *
 * @param a pointer to the first ColorZone
 * @param b pointer to the second ColorZone
 *
 * @return -1 if a < b, 1 if a > b, 0 if equal
 */
static int ruler_core_color_zone_location_cmp(const void *a, const void *b)
{
    ColorZone *zone_a = (ColorZone*)a;
    ColorZone *zone_b = (ColorZone*)b;

    if(zone_a->from < zone_b->from) return -1;
    if(zone_a->from > zone_b->from) return 1;
    return 0;
}

/**
 * @brief Helper function to sort ColorZones by their 'divider' value.
 *
 * This helps drawing the bigger zones first, so that smaller zones will
 * appear to be drawn on top of them.
 *
 * @param a pointer to the first ColorZone
 * @param b pointer to the second ColorZone
 *
 * @return -1 if a < b, 1 if a > b, 0 if equal
 */
static int ruler_core_color_zone_divider_cmp(const void *a, const void *b)
{
    ColorZone *zone_a = (ColorZone*)a;
    ColorZone *zone_b = (ColorZone*)b;

    if(zone_a->divider < zone_b->divider) return -1;
    if(zone_a->divider > zone_b->divider) return 1;
    return 0;
}
