/*
 * SPDX-FileCopyrightText: 2026 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef RULER_CORE_H
#define RULER_CORE_H

#include "generic-strip.h"
#include "SDL_pcf.h"

/* /!\ do not change that without rebuilding*/
/*That should be enough for anybody :)*/
#define RULER_CORE_MAX_TICKS 4
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
    Top    =  1 << 0, /*00000001*/
    Center =  1 << 1, /*00000010*/
    Bottom =  1 << 2, /*00000100*/
    Left   =  1 << 3, /*00001000*/
    Right  =  1 << 4, /*00010000*/
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
    int divider; /**< The divider of the fill ratio, used to stack overlapping zones. */
}ColorZone;

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
 * @brief Anchor point structure. Maps a value to a pixel position on the ruler.
 *
 */
typedef struct{
    float value; /**< Numeric value on the ruler. */
    unsigned int position; /**< Pixel position of the value on the vertical/horizontal axis (ruler coordinates). */
}Anchor;

/**
 * @brief The RulerCore struct/object. See generic-ruler.c for a
 * discussion on how it works
 */
typedef struct {
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
    TickMark ticks[RULER_CORE_MAX_TICKS];
    /**
     * @brief Location of the first value that bears a tick mark.
     */
    Anchor first_tick;
    int spine_coord;
}RulerCore;

#define RULER_CORE(obj) ((RulerCore *)(obj))

#define RULER_CORE_DEFAULT_TEXT_MARGIN 4


typedef bool (*WillDrawTickFunc)(RulerCore *self, GenericStrip *strip,
                                 float value, int *tick_size, Uint32 *color);
typedef bool (*DrawTickLabelFunc)(RulerCore *self, GenericStrip *strip,
                                  float value, uint8_t precision,
                                  PCF_Font *font, Uint32 color,
                                  int x, int y, PCF_TextPlacement placement);


/**
 * @brief Helper function to get the minimum pixel coordinate (in image
 * coordinates) of the ruler's axis (x or y depending if the ruler is horizontal
 * or vertical).
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 *
 * @return The minimum pixel coordinate of the ruler's axis.
 */
static inline int ruler_core_axis_min(RulerCore *self, GenericStrip *strip)
{
    return generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
         ? self->ruler_area.x
         : self->ruler_area.y;
}

/**
 * @brief Helper function to get the maximum pixel coordinate (in image
 * coordinates) of the ruler's axis (x or y depending if the ruler is horizontal
 * or vertical).
 *
 * @param self a RulerCore
 * @param strip the GenericStrip that's worked on
 *
 * @return The maximum pixel coordinate of the ruler's axis.
 */
static inline int ruler_core_axis_max(RulerCore *self, GenericStrip *strip)
{
    return generic_strip_orientation(strip, ORIENTATION_HORIZONTAL)
         ? SDLExt_RectLastX(&(self->ruler_area))
         : SDLExt_RectLastY(&(self->ruler_area));
}

bool ruler_core_set_ticks(RulerCore *self, float base_unit, uint16_t base_unit_px,
                          uint8_t base_unit_size, bool base_unit_labeled,
                          uint8_t nmultiples, RulerUnitMultiple *multiples);
void ruler_core_set_anchor(RulerCore *self, GenericStrip *strip, Anchor anchor);
int ruler_core_get_spine_coord(RulerCore *self, RulerOrientation orientation,
                               Location spine_location);


float ruler_core_position_to_value(RulerCore *self, GenericStrip *strip, unsigned int px_position);
float ruler_core_increment_to_value(RulerCore *self, GenericStrip *strip, unsigned int px_increment);
int ruler_core_get_pixel_increment_for(RulerCore *self, GenericStrip *strip, float value);

bool ruler_core_draw_ticks(RulerCore *self, GenericStrip *strip,
                             Uint32 color, bool etch_spine, Location spine_location,
                             WillDrawTickFunc will_draw_tick);
bool ruler_core_draw_labels(RulerCore *self, GenericStrip *strip,
                              Location markings_location, int text_margin,
                              PCF_Font *font, Uint32 color, uint8_t precision,
                              DrawTickLabelFunc draw_tick_label);
bool ruler_core_draw_tick_label_impl(RulerCore *self, GenericStrip *strip,
                                     float value, uint8_t precision,
                                     PCF_Font *font, Uint32 color, int x,
                                     int y, PCF_TextPlacement placement);

void ruler_core_preprocess_color_zones(int nzones, ColorZone *zones);
bool ruler_core_draw_zones(RulerCore *self, GenericStrip *strip,
                           Location spine_location, int nzones, ColorZone *zones,
                           float fill_ratio);


#endif /* RULER_CORE_H */
