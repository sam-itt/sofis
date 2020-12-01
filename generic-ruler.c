#include <stdio.h>
#include <stdlib.h>

#include "generic-ruler.h"

#include "SDL_surface.h"
#include "generic-layer.h"
#include "misc.h"
#include "view.h"

#define TEXT_SPACE 4


bool generic_ruler_get_size_request(GenericRuler *self, int8_t precision, PCF_Font *font, Location markings_location, int *w, int *h);

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
 *  0       25    2      NO   25 = 12.5 * 2 -> not a integeral number of marks
 *
 * @param start First (lower) value of the range to map.
 * @param end Last(highier) value of the range to map
 * @param step Hatch marks spacing. e.g. from = 0, to = 25, step = 5 will
 * map a range from 0 to 25 with marks at every 5 units to @p ruler_w pixels if
 * @p orientation is RulerHorizontal and to @p ruler_h pixels if orientation is
 * RulerVertical. A negative value will disable intermediate hatching.
 * @param marks_font The PCF_Font that will be used for markings. NULL If no
 * markings are to be drawn.
 * @param marks_location Where the markings will be drawn, used to compute the
 * full dimensions needed by the ruler itself plus its eventual markings.
 * @param markings_precision How many digital places will be written for
 * each marking. 0 For integral part only.
 * @param ruler_w Maximum width of the ruler, can be modified to represent the
 * range and accomodate for markings. Pass NONE if you aren't goint to use the
 * markings-etching features of GenericRuler.
 * @param ruler_h Maximum height of the ruler, can be modified to represent the
 * range and accomodate for markings
 *
 * @see draw_markings
 *
 */
GenericRuler *generic_ruler_init(GenericRuler *self,
                                 RulerOrientation orientation,
                                 RulerDirection direction,
                                 float start, float end, float step,
                                 PCF_Font *markings_font,
                                 Location markings_location,
                                 int8_t markings_precision,
                                 int ruler_w, int ruler_h)
{
    int fullw, fullh;

    self->orientation = orientation;
    self->direction = direction;
    self->ruler_area.w = ruler_w;
    self->ruler_area.h = ruler_h;

    self->start = start;
    self->end = end;
    if(self->end < self->start){
        printf("%s: self->end(%f) must be greather than self->start(%f).",
            __FUNCTION__, self->end, self->start
        );
        return NULL;
    }

    self->hatch_step = (step > 0) ? step : self->end;
    if(fmodf(self->end,self->hatch_step) != 0){
        printf("Warning: range end (%f) is not a multiple of hatch_step(%f), rendering will break\n",
            self->end,
            self->hatch_step
        );
    }

    /*Sophie's algorithm*/
    int nintervals = (self->end - self->start) / self->hatch_step;
    int nbars = nintervals + 1;
    int pix_available = (self->orientation == RulerHorizontal) ?
                          ruler_w - (nbars*1) // each bar is 1px wide/tall
                        : ruler_h - (nbars*1);
    int interval_pix_size = pix_available/nintervals; //This is the best interval(number of pixels BETWEEN two etches) size in px.

    /* interval pix size is the number of pixels between bars,
     * +1 includes the closing bar */
    self->ppv = (interval_pix_size+1) / self->hatch_step;
    if(self->orientation == RulerHorizontal)
        self->ruler_area.w = interval_pix_size * nintervals + (nbars*1); //Each bar is 1px wide
    else
        self->ruler_area.h = interval_pix_size * nintervals + (nbars*1);

    if(markings_font != NULL && markings_location != LocationNone){
        generic_ruler_get_size_request(self, markings_precision, markings_font, markings_location, &fullw, &fullh);
    }else{
        fullw = self->ruler_area.w;
        fullh = self->ruler_area.h;
    }

    generic_layer_init(GENERIC_LAYER(self), fullw, fullh);

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
 * @warning This functions modifies self->ruler_area.
 *
 * @param self a GenericRuler
 * @param font a The PCF_Font that'll be used for markings
 * @param markings_location Where the markings will be put relatively to
 * the ruler
 * @param w Where to put the computed needed width
 * @param h Where to put the computed needed height
 * @return true on success, false otherwise
 *
 */
bool generic_ruler_get_size_request(GenericRuler *self, int8_t precision, PCF_Font *font, Location markings_location, int *w, int *h)
{
    int pixel_increment = roundf(self->hatch_step * self->ppv);

    int glyph_height = PCF_FontCharHeight(font);

    char vbuffer[10]; /*9999999999 or 999999999 with a floating dot*/
    SDL_Rect m_rect; //mark rect

    /*Pixels to add on each side of the Fishbone to
     * accomodate etching marks
     * */
    int left, right;
    int top, bottom;

    int fbw = self->ruler_area.w;
    int fbh = self->ruler_area.h;

    left = right = top = bottom = 0;
    if(self->orientation == RulerHorizontal){
        if(self->direction == RulerGrowAlongAxis){
            snprintf(vbuffer, 10, "%*g", precision, self->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            left = SDLExt_RectMidX(&m_rect) - m_rect.x; //We need 'left' more pixels on the left

            snprintf(vbuffer, 10, "%*g", precision, self->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            right = SDLExt_RectLastX(&m_rect) - SDLExt_RectMidX(&m_rect);//We need 'right' more pixels on the right
        }else{
            snprintf(vbuffer, 10, "%*g", precision, self->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            left = SDLExt_RectMidX(&m_rect) - m_rect.x; //We need 'left' more pixels on the left

            snprintf(vbuffer, 10, "%*g", precision, self->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            right = SDLExt_RectLastX(&m_rect) - SDLExt_RectMidX(&m_rect);//We need 'right' more pixels on the right
        }

        if(markings_location == Top){
            top = TEXT_SPACE + glyph_height;
        }else if (markings_location == Bottom){
            bottom = TEXT_SPACE + glyph_height;
        }else{
            printf("Unsupported Etching location for Horizontal orientation\n");
            return false;
        }
    }else if(self->orientation == RulerVertical){
        if(self->direction == RulerGrowAlongAxis){
            snprintf(vbuffer, 10, "%*g", precision, self->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            top = SDLExt_RectMidY(&m_rect) - m_rect.y;

            snprintf(vbuffer, 10, "%*g", precision, self->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            bottom = SDLExt_RectLastY(&m_rect) - SDLExt_RectMidY(&m_rect);
        }else{
            snprintf(vbuffer, 10, "%*g", precision, self->start);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            bottom = SDLExt_RectLastY(&m_rect) - SDLExt_RectMidY(&m_rect);

            snprintf(vbuffer, 10, "%*g", precision, self->end);
            PCF_FontGetSizeRequestRect(font, vbuffer, &m_rect);
            top = SDLExt_RectMidY(&m_rect) - m_rect.y;
        }

        /* Assumes that self->end > self->start
         * means that it's also the lenghtiest to
         * write. It also just happens that we've
         * just measured it's size in m_rect.
         * */
        if(markings_location == Left){
            left = m_rect.w + TEXT_SPACE;
        }else if (markings_location == Right) {
            right = m_rect.w + TEXT_SPACE;
        }else{
            printf("Unsupported Etching location for Vertical orientation\n");
            return false;
        }
    }else{
        printf("Unsupported orientation\n");
        return false;
    }

//    printf("Width was %d, it's now %d\n", , fbw + left + right);
    *w = fbw + left + right;
    *h = fbh + top + bottom;
    printf("Passed-in fbw: %d, returning w: %d\n",fbw, *w);
    printf("ruler_area.w was: %d is now: %d\n",self->ruler_area.w, fbw);

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
    printf("Final gauge area:\n");
    SDLExt_RectDump(&self->ruler_area);
    printf("\n");
    return true;
}

int generic_ruler_get_pixel_increment_for(GenericRuler *self, float value)
{
    if(value == -INFINITY) return 0;
    if(value == INFINITY)
        value = self->end;

    if(fmod(value, self->hatch_step) == 0){ /*Value is a big graduation*/
//        value = fmod(value, fabs(round(self->end - self->start)) + 1);
        int ngrads = value/self->hatch_step - (self->start/self->hatch_step);
        return ngrads * self->ppv * self->hatch_step;
    }else{
        if(self->orientation == RulerHorizontal)
            return (value - self->start)*((self->ruler_area.w-1) - 0)/(self->end - self->start) + 0;
        else if(self->orientation == RulerVertical)
            return (value - self->start)*((self->ruler_area.h-1) - 0)/(self->end - self->start) + 0;
        else
            printf("Unsupported orientation\n");
    }
    return -1;
}

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

    if(self->orientation == RulerHorizontal){
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
            printf("Unsupported line position for Horizontal orientation");
            return false;
        }
    }else if(self->orientation == RulerVertical){
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
            printf("Unsupported line position for Vertical orientation");
            return false;
        }
    }else{
            printf("Unsupported gauge orientation");
            return false;
    }

    /* Colored areas, if any are assumed to be sorted
     * */
    SDL_LockSurface(GENERIC_LAYER(self)->canvas);
    Uint32 *pixels = GENERIC_LAYER(self)->canvas->pixels;
    for(int i = 0; i < nzones; i++){
        begin = generic_ruler_get_pixel_increment_for(self, zones[i].from);
        end = generic_ruler_get_pixel_increment_for(self, zones[i].to);
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
        if(self->orientation == RulerHorizontal){
            for(int y = start_y; y <= end_y; y++){
                if(self->direction == RulerGrowAlongAxis){
                    for(int x = self->ruler_area.x + begin; x <= self->ruler_area.x + end; x++){
                        pixels[y * GENERIC_LAYER(self)->canvas->w + x] = bcolor;
                    }
                }else{
                    for(int x = SDLExt_RectLastX(&self->ruler_area) - end; x <= SDLExt_RectLastX(&self->ruler_area) - begin; x++){
                        pixels[y * GENERIC_LAYER(self)->canvas->w + x] = bcolor;
                    }
                }
            }
        }else if(self->orientation == RulerVertical){
            if(self->direction == RulerGrowAlongAxis){
                for(int y = self->ruler_area.y + begin; y <= self->ruler_area.y + end; y++){
                    for(int x = start_x; x <= end_x; x++){
                        pixels[y * GENERIC_LAYER(self)->canvas->w + x] = bcolor;
                    }
                }
            }else{
                for(int y = SDLExt_RectLastY(&self->ruler_area) - end; y <= SDLExt_RectLastY(&self->ruler_area) - begin; y++){
                    for(int x = start_x; x <= end_x; x++){
                        pixels[y * GENERIC_LAYER(self)->canvas->w + x] = bcolor;
                    }
                }
            }
        }
    }
    SDL_UnlockSurface(GENERIC_LAYER(self)->canvas);
    return true;
}

/**
 * @brief Write ruler hatches(lines) on the underlying canvas.
 *
 * @param self a GenericRuler
 * @param color The color to use for hatches, in the underlying surface format
 * @param etch_spine Whether (true/false) to draw the ruler's spine i.e. main
 * line or not.
 * @param etch_hatches Whether (true/false) to draw hatches i.e. unit lines
 * perpendicular to the spine or not.
 * @param spine_location Where to draw the spine. Valid locations for
 * Horizontal gauges are Top,Center,Bottom and Left, Center, Right for Vertical
 * gauges. Ignored when @p spine is false.
 * @return true on success, false otherwise.
 *
 *
 */
bool generic_ruler_etch_hatches(GenericRuler *self, Uint32 color, bool etch_spine, bool etch_hatches, Location spine_location)
{
    Uint32 *pixels;
    int pcursor; /*Pixel index cursor*/
    int increment;

    generic_layer_lock(GENERIC_LAYER(self));
    pixels = GENERIC_LAYER(self)->canvas->pixels;
    if(self->orientation == RulerHorizontal){
        /*Spine (main line)*/
        if(etch_spine){
            int y = -1;
            if(spine_location == Center)
                y = SDLExt_RectMidY(&self->ruler_area);
            else if(spine_location == Bottom)
                y = SDLExt_RectLastY(&self->ruler_area);
            else if(spine_location == Top)
                y = self->ruler_area.y;
            else
                printf("Unsupported line position for Horizontal orientation\n");
            if(y < 0)
                return false; //TODO:Unlock pixels
            for(int x = self->ruler_area.x; x <= SDLExt_RectLastX(&self->ruler_area); x++)
                pixels[y * GENERIC_LAYER(self)->canvas->w + x] = color;
        }
        /*Hatch marks*/
        if(etch_hatches){
            for(float i = self->start; i <= self->end; i += self->hatch_step){
                increment = generic_ruler_get_pixel_increment_for(self, i);
                pcursor = (self->direction == RulerGrowAlongAxis)
                          ? self->ruler_area.x + increment
                          : SDLExt_RectLastX(&self->ruler_area) - increment;
                printf("Etching value %d at x=%d (increment was %d)\n", (int)i, pcursor, increment);
                for(int y = self->ruler_area.y; y <= SDLExt_RectLastY(&self->ruler_area); y++)
                    pixels[y * GENERIC_LAYER(self)->canvas->w + pcursor] = color;
            }
        }
    }else if(self->orientation == RulerVertical) {
        /*Spine (main line)*/
        if(etch_spine){
            int x = -1;
            if(spine_location == Center)
                x = SDLExt_RectMidX(&self->ruler_area);
            else if(spine_location == Left)
                x = self->ruler_area.x;
            else if(spine_location == Right)
                x = SDLExt_RectLastX(&self->ruler_area);
            else
                printf("Unsupported line position for Vertical orientation\n");
            if(x < 0)
                return false; //TODO:Unlock pixels
            for(int y = self->ruler_area.y; y <= SDLExt_RectLastY(&self->ruler_area); y++)
                pixels[y * GENERIC_LAYER(self)->canvas->w + x] = color;
        }
        /*Hatch marks*/
        if(etch_hatches){
            for(float i = self->start; i <= self->end; i += self->hatch_step){
                increment = generic_ruler_get_pixel_increment_for(self, i);
                pcursor = (self->direction == RulerGrowAlongAxis)
                          ? self->ruler_area.y + increment
                          : SDLExt_RectLastY(&self->ruler_area) - increment;
                printf("Drawing %f hatch at y = %d\n",i, pcursor);
                for(int x = self->ruler_area.x; x < self->ruler_area.x + self->ruler_area.w; x++){
                    pixels[pcursor * GENERIC_LAYER(self)->canvas->w + x] = color;
                }
            }
        }
    }else{
        printf("Unsupported orientation\n");
        return false; //TODO:Unlock pixels
    }
    generic_layer_unlock(GENERIC_LAYER(self));
    return true;
}

/**
 * @brief Etch markings (numbers) on the ruler at the predefined
 * step
 *
 * @param self a GenericRuler
 * @param location Where to put the markings. Valid locations for Horizontal
 * rulers are: Top, Bottom while valid locations for Vertical gauges are
 * Left and Right.
 * @param font The PCF_Font to use. Must be the same used when calling
 * generic_ruler_get_size_request
 * @param color The color to use, in the underlying surface format
 * @return true on success, false otherwise.
 *
 * @see generic_ruler_get_size_request
 */
/*TODO: Handle values ndigits vs precision ints vs floats*/
bool generic_ruler_etch_markings(GenericRuler *self, Location markings_location, PCF_Font *font, Uint32 color, int8_t precision)
{
    int pcursor; /*Pixel cursor*/
    int increment;

    if(self->orientation == RulerHorizontal){
        for(float i = self->start; i <= self->end; i += self->hatch_step){
            increment = generic_ruler_get_pixel_increment_for(self, i);
            pcursor = (self->direction == RulerGrowAlongAxis)
                      ? self->ruler_area.x + increment
                      : SDLExt_RectLastX(&self->ruler_area) - increment;
            if(markings_location == Bottom)
                PCF_FontWriteNumberAt(font,
                    &i, TypeFloat, precision,
                    color, GENERIC_LAYER(self)->canvas,
                    pcursor,
                    self->ruler_area.h + TEXT_SPACE,
                    CenterOnCol | BelowRow
                );
            else if(markings_location == Top)
                PCF_FontWriteNumberAt(font,
                    &i, TypeFloat, precision,
                    color, GENERIC_LAYER(self)->canvas,
                    pcursor,
                    self->ruler_area.y - TEXT_SPACE,
                    CenterOnCol | AboveRow
                );
            else{
                printf("Unsupported etch position for horizontal scale\n");
                return false;
            }
        }
    }else if(self->orientation == RulerVertical){
        for(float i = self->start; i <= self->end; i += self->hatch_step){
            increment = generic_ruler_get_pixel_increment_for(self, i);
            printf("increment for %f is %d\n",i, increment);
            pcursor = (self->direction == RulerGrowAlongAxis)
                      ? self->ruler_area.y + increment
                      : SDLExt_RectLastY(&self->ruler_area) - increment;
            if(markings_location == Right)
                PCF_FontWriteNumberAt(font,
                    &i, TypeFloat, precision,
                    color, GENERIC_LAYER(self)->canvas,
                    self->ruler_area.w + TEXT_SPACE,
                    pcursor,
                    RightToCol | CenterOnRow
                );
            else if(markings_location == Left)
                PCF_FontWriteNumberAt(font,
                    &i, TypeFloat, precision,
                    color, GENERIC_LAYER(self)->canvas,
                    self->ruler_area.x - TEXT_SPACE,
                    pcursor,
                    LeftToCol | CenterOnRow
                );
            else{
                printf("Unsupported etch position for vertical scale\n");
                return false;
            }
        }
    }else{
        printf("Unsupported orientation\n");
        return false;
    }
    return true;
}

void generic_ruler_clip_value(GenericRuler *self, float *value)
{
    if(*value > self->end)
        *value = self->end;
    if(*value < self->start)
        *value = self->start;
}
