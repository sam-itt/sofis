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

#include "SDL_pcf.h"
#include <stdint.h>

/**
 * @brief Ruler orientation.
 *
 * These values are used to specify the orientation of a GenericRuler.
 */
typedef enum __attribute__ ((__packed__)){
    RulerVertical =  ORIENTATION_VERTICAL, /**< The ruler is vertical. */
    RulerHorizontal = ORIENTATION_HORIZONTAL, /**< The ruler is horizontal. */
}RulerOrientation;

/**
 * @brief Ruler direction.
 *
 * These values are used to specify the direction of a GenericRuler.
 */
typedef enum __attribute__ ((__packed__)){
    RulerGrowAlongAxis = GROW_ALONG_AXIS,    /**< Grows left to right, top to bottom. */
    RulerGrowAgainstAxis = GROW_AGAINST_AXIS,  /**< Values grow in reverse with respect to the axis*/
}RulerDirection;



typedef enum __attribute__ ((__packed__)){
    LocationNone = 0,
    Top,
    Center,
    Bottom,
    Left,
    Right,
    NLocations
}Location;

/**
 * @brief Bound flags for color zones.
 *
 * These flags are used to indicate whether the from/to bounds of a ColorZone
 * are included or excluded in the range.
 *
 * @see ColorZone
 * TODO: Consider using only 2 bits so that mutually exclusive flags cannot be
 * set as it is now.
 */
typedef enum __attribute__ ((__packed__)){
    FromIncluded = 1 << 0, /*00000001*/
    FromExcluded = 1 << 1, /*00000010*/
    ToIncluded =   1 << 2, /*00000100*/
    ToExcluded =   1 << 3  /*00001000*/
}BoundFlags;

/**
 * @brief Color zone structure.
 *
 * This structure is used as arguments to generic_ruler_draw_zones to define
 * colored extents on a GenericRuler.
 */
typedef struct{
    float from; /**< The value at which the color zone starts. */
    float to; /**< The value at which the color zone ends. */
    BoundFlags flags; /**< Flags to indicate whether the from/to bounds are included or excluded in the range */
    SDL_Color color; /**< The color of the zone. */
}ColorZone;

/**
 * @brief Structure that holds params relative to unit multiples
 *
 * This type is used to as arguments of generic_ruler_init to represent the
 * multiples of the base_unit of that call. The values given here will be usedstorestore
 * to compute the actual TickMarkSpecs of the inited GenericRuler
 */
typedef struct{
    uint8_t value; /**< One mark every X base_unit. */
    uint8_t size; /**< how tall/wide the tick should be, in pixels.*/
    bool labeled; /**< Whether the tick will have a marking or not. */
}RulerUnitMultiple;

/**
 * @brief Structure that describe tick marks "levels" on the ruler.
 *
 * This structure is used to store the properties of a tick mark on a
 * GenericRuler (each X unit, e.g. each 1/4", each 1/2", ...). Tick marks are
 * always multiples of the base unit, which is specified in the GenericRuler.
 */
typedef struct{
    float value; /**< This tick mark will be drawn each TickMarkSpec#value units, e.g. (each 1/4", each 1/2", ...). */
    uint8_t stride; /**< increment in pixels from on tick to another. */
    uint8_t size; /**< how tall/wide the tick should be, in pixels. */
    bool labeled; /**< Whether the tick will have a marking or not. */
}TickMark;


/**
 * @brief Anchor point structure. Maps a value to a pixel position on the ruler.
 *
 */
typedef struct{
    float value; /**< Numeric value on the ruler. */
    unsigned int position; /**< Pixel position of the value on the vertical/horizontal axis (ruler coordinates). */
}Anchor;

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
     *  GenericRuler provides a default implementation of that,
     *  generic_ruler_etch_tick_label_impl that will just write the value
     *  verbatim. Subclasses can chain call it as needed. Callers will invoke
     *  generic_ruler_etch_tick_label which will trampoline to the
     *  implementation provided through that structure during init.
     *
     *  @see generic_ruler_etch_tick_label_impl
     *  @see generic_ruler_etch_tick_label
     *  @see generic_ruler_init
     */
    bool (*etch_tick_label)(GenericRuler *self, float value, uint8_t precision,
                            PCF_Font *font, Uint32 color,
                            int x, int y, PCF_TextPlacement placement);
    /**
     * @brief Pre-etch callback to let subclasses change parameters
     *
     * This function is called before etching a tick mark on the ruler. It
     * allows subclasses to change the tick size and color based on the value of
     * the tick.
     */
    bool (*will_etch_tick)(GenericRuler *self, float value, int *tick_size, Uint32 *color);
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

    /**
     * @brief The area of the ruler within the canvas.
     *
     * If we need to have the canvas bigger than the ruler to hold markings that
     * are out of bounds of the actual ruler, this will define the area of the
     * ruler within the canvas. It'll be the base for ruler coordinate that are
     * expressed in increments from its start.
     */
    SDL_Rect ruler_area;

    /** @brief Number of tick marks, including the base unit.
     *
     * The first tick is always the base unit, and the rest are multiples of it.
     */
    uint8_t nticks;
    /**
     * @brief The tick marks, including the base unit (first one).
     */
    TickMark ticks[4]; /*That should be enough for anybody :)*/
    /**
     * @brief Location of the first value that bears a tick mark.
     */
    Anchor first_tick;
    uint32_t spine_coord;
}GenericRuler;

#define GENERIC_RULER(obj) ((GenericRuler *)(obj))
#define GENERIC_RULER_DEFAULT_TEXT_MARGIN 4

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
    return ((GENERIC_STRIP(self)->flags & ORIENTATION_MASK) == orientation);
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
    return ((GENERIC_STRIP(self)->flags & DIRECTION_MASK) == direction);
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
    return generic_ruler_orientation(self, RulerHorizontal)
         ? self->ruler_area.x
         : self->ruler_area.y;
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
    return generic_ruler_orientation(self, RulerHorizontal)
         ? SDLExt_RectLastX(&(self->ruler_area))
         : SDLExt_RectLastY(&(self->ruler_area));
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
                                  float base_unit, uint16_t base_unit_px,
                                  uint8_t base_unit_size, bool base_unit_labeled,
                                  uint8_t nmultiples, RulerUnitMultiple *multiples,
                                  Anchor anchor);

float generic_ruler_position_to_value(GenericRuler *self, unsigned int px_position);
float generic_ruler_increment_to_value(GenericRuler *self, unsigned int px_increment);

bool generic_ruler_etch_hatches(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location);
bool generic_ruler_etch_markings(GenericRuler *self, Location markings_location, int text_margin, PCF_Font *font, Uint32 color, uint8_t precision);

bool generic_ruler_draw_hatches(GenericRuler *self, Uint32 color, bool etch_spine, Location spine_location, int preload_px);
bool generic_ruler_draw_zones(GenericRuler *self, Location spine_location, int nzones, ColorZone *zones, float fill_ratio);

/**
 * @brief Function wrapper to draw a tick label (text) on the ruler.
 *
 * This "trampoline" function will call the implementation provided by the
 * specific GenericRuler subclass, if any. If no implementation is provided,
 * it will return false. GenericRuler comes with a default implementation that
 * will just write the value verbatim. Subclasses can change the params and
 * chain call to that.
 *
 * @param self a GenericRuler
 * @param value The value to write.
 * @param precision How many digital places to write. 0 for integral part only.
 * @param font The PCF_Font to use.
 * @param color The color to use, in the underlying surface format
 * @param x The x coordinate of the text placement (in image coordinates)
 * @param y The y coordinate of the text placement (in image coordinates)
 * @param placement @see PCF_TextPlacement
 *
 * @return true on success, false otherwise.
 *
 * @see generic_ruler_etch_tick_label_impl
 */
static inline bool generic_ruler_etch_tick_label(GenericRuler *self, float value, uint8_t precision,
                                                 PCF_Font *font, Uint32 color,
                                                 int x, int y, PCF_TextPlacement placement)
{
    return self->ops->etch_tick_label ? self->ops->etch_tick_label(self, value, precision, font, color, x, y, placement) : false;
}

bool generic_ruler_etch_tick_label_impl(GenericRuler *self, float value, uint8_t precision,
                                        PCF_Font *font, Uint32 color,
                                        int x, int y, PCF_TextPlacement placement);


int generic_ruler_get_pixel_increment_for(GenericRuler *self, float value);
bool generic_ruler_get_size_request(GenericRuler *self, int8_t precision, PCF_Font *font, Location markings_location, int text_margin, int *w, int *h);

void generic_ruler_dispose(GenericRuler *self);

#endif /* GENERIC_RULER_H */
