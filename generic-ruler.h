/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef GENERIC_RULER_H
#define GENERIC_RULER_H

#include "generic-strip.h"
#include "ruler-core.h"

#include "SDL_pcf.h"
#include <stdint.h>

typedef struct _GenericRuler GenericRuler;

/**
 * @brief VTable for GenericRuler virtuals.
 *
 * This structure defines the virtual functions for a GenericRuler. Subclasses
 * can provide their own implementations of these functions to customize the
 * behavior of the ruler. The current operations available for customization let
 * subclasses customize the ticks and labels drawing behavior.
 */
typedef struct{
    /** @brief The function used to draw a label(numeric value on the ruler).
     *
     *  GenericRuler provides a default implementation of that, through
     *  ruler_core_draw_tick_label_impl, that will just write the value verbatim.
     *  Subclasses can chain call it as needed. Callers will invoke
     *  ruler_core_draw_tick_label_impl which will trampoline to the
     *  implementation provided through that structure during init.
     *
     *  You can find an example on how to use that in vertical-speed-indicator.c
     *
     *  @see ruler_core_draw_tick_label_impl
     *  @see generic_ruler_init
     *  @see vertical_speed_indicator_will_draw_tick
     *  @see vertical_speed_indicator_draw_tick_label
     *  @see vertical_speed_indicator_init
     *  @see vertical_speed_indicator_ruler_ops
     */
    bool (*draw_tick_label)(GenericRuler *self, float value, uint8_t precision,
                            PCF_Font *font, Uint32 color,
                            int x, int y, PCF_TextPlacement placement);
    /**
     * @brief Pre-draw callback to let subclasses change parameters
     *
     * This function is called before drawing a tick mark on the ruler. It
     * allows subclasses to change the tick size and color based on the value of
     * the tick.
     */
    bool (*will_draw_tick)(GenericRuler *self, float value, int *tick_size, Uint32 *color);
}GenericRulerOps;
/**
 * @brief The main GenericRuler struct/object. See generic-ruler.c for a
 * discussion on how it works
 */
typedef struct _GenericRuler{
    /**
     * @brief Parent that provides orientation-agnostic mapping between a range
     * of values and a range of pixels.
     */
    GenericStrip super;
    GenericRulerOps *ops; /**< Customizable operations, @see GenericRulerOps. */

    RulerCore core; /**< The core that provides the mapping between a range of values and a range of pixels. */
}GenericRuler;

#define GENERIC_RULER(obj) ((GenericRuler *)(obj))

/**
 * @brief Helper function to determine the orientation of a GenericRuler.
 *
 * @param self a GenericRuler
 * @param orientation The orientation to check for.
 *
 * @return true if the GenericRuler has the specified orientation, false otherwise.
 */
static inline bool generic_ruler_orientation(GenericRuler *self, RulerOrientation orientation)
{
    return generic_strip_orientation(GENERIC_STRIP(self), (StripFlags)orientation);
}

/**
 * @brief Helper function to determine the direction of a GenericRuler.
 *
 * @param self a GenericRuler
 * @param direction The direction to check for.
 *
 * @return true if the GenericRuler has the specified direction, false otherwise.
 */
static inline bool generic_ruler_direction(GenericRuler *self, RulerDirection direction)
{
    return generic_strip_direction(GENERIC_STRIP(self), (StripFlags)direction);
}

/**
 * @brief Helper function to get the minimum pixel coordinate (in image
 * coordinates) of the ruler's axis (x or y depending if the ruler is horizontal
 * or vertical).
 *
 * @param self a GenericRuler
 *
 * @return The minimum pixel coordinate of the ruler's axis.
 */
static inline int generic_ruler_axis_min(GenericRuler *self)
{
    return ruler_core_axis_min(&(self->core), GENERIC_STRIP(self));
}

/**
 * @brief Helper function to get the maximum pixel coordinate (in image
 * coordinates) of the ruler's axis (x or y depending if the ruler is horizontal
 * or vertical).
 *
 * @param self a GenericRuler
 *
 * @return The maximum pixel coordinate of the ruler's axis.
 */
static inline int generic_ruler_axis_max(GenericRuler *self)
{
    return ruler_core_axis_max(&(self->core), GENERIC_STRIP(self));
}

GenericRuler *generic_ruler_init_from_size(GenericRuler *self,
                                           RulerOrientation orientation,
                                           RulerDirection direction,
                                           float start, float end, float step,
                                           uint8_t tick_size,
                                           PCF_Font *markings_font,
                                           Location markings_location,
                                           int text_margin,
                                           int8_t markings_precision,
                                           int ruler_w, int ruler_h);

GenericRuler *generic_ruler_init(GenericRuler *self,
                                  int width, int height,
                                  RulerOrientation orientation,
                                  RulerDirection direction,
                                  Location spine_location,
                                  float base_unit, uint16_t base_unit_px,
                                  uint8_t base_unit_size, bool base_unit_labeled,
                                  uint8_t nmultiples, RulerUnitMultiple *multiples,
                                  Anchor anchor);

bool generic_ruler_get_size_request(GenericRuler *self, int8_t precision,
                                    PCF_Font *font, Location markings_location,
                                    int text_margin, int *w, int *h);

void generic_ruler_dispose(GenericRuler *self);

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
static inline float generic_ruler_position_to_value(GenericRuler *self, unsigned int px_position)
{
    return ruler_core_position_to_value(&(self->core), GENERIC_STRIP(self), px_position);
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
static inline float generic_ruler_increment_to_value(GenericRuler *self, unsigned int px_increment)
{
    return ruler_core_increment_to_value(&(self->core), GENERIC_STRIP(self), px_increment);
}

bool generic_ruler_draw_ticks(GenericRuler *self, Uint32 color,
                              bool etch_spine, Location spine_location);

bool generic_ruler_draw_labels(GenericRuler *self, Location markings_location,
                               int text_margin, PCF_Font *font,
                               Uint32 color, uint8_t precision);

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
 * @param zones Address to the first of @p nzones zones. These zones *must* have
 * been preprocessed by generic_ruler_preprocess_color_zones.
 * @param fill_ratio amount of the ruler to fill with colors, from 0 (0%)
 * to 1.0 (100%). 100% is as high as the longest tick mark, 0% is no fill at all.
 * @return true on success, false otherwise.
 *
 * @see ruler_core_preprocess_color_zones
 */
static inline bool generic_ruler_draw_zones(GenericRuler *self, Location spine_location,
                                            int nzones, ColorZone *zones,
                                            float fill_ratio)
{
    return ruler_core_draw_zones(
        &(self->core), GENERIC_STRIP(self), spine_location, nzones,
        zones, fill_ratio
    );
}

/**
 * @brief Given a value, return the pixel increment (relative to the ruler's
 * start coordinate) that represents it on the ruler.
 *
 * Warning: This function does not check if the value is within the range of
 * the ruler. If the value is outside the ruler's range, it will return a
 * mathematically correct pixel increment that will be outside of the canvas
 *
 * @param self a GenericRuler
 * @param value The value to translate, assumed to be between
 * GENERIC_STRIP(self)->begin and GENERIC_STRIP(self)->begin.
 *
 * @return The corresponding pixel increment.
 */
static inline int generic_ruler_get_pixel_increment_for(GenericRuler *self, float value)
{
    return ruler_core_get_pixel_increment_for(&self->core, GENERIC_STRIP(self), value);
}


#endif /* GENERIC_RULER_H */
