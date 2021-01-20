#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "SDL_pcf.h"

#include "attitude-indicator.h"
#include "base-animation.h"
#include "base-gauge.h"
#include "generic-layer.h"
#include "misc.h"
#include "resource-manager.h"
#include "roll-slip-gauge.h"
#include "sdl-colors.h"

#define sign(x) (((x) > 0) - ((x) < 0))

static void attitude_indicator_render(AttitudeIndicator *self, Uint32 dt, RenderContext *ctx);
static void attitude_indicator_update_state(AttitudeIndicator *self, Uint32 dt);
static BaseGaugeOps attitude_indicator_ops = {
   .render = (RenderFunc)attitude_indicator_render,
   .update_state = (StateUpdateFunc)attitude_indicator_update_state
};

static SDL_Surface *attitude_indicator_get_etched_ball(AttitudeIndicator *self);
static SDL_Surface *attitude_indicator_draw_ruler(AttitudeIndicator *self, int size, int ppm, PCF_Font *font, SDL_Color *col);

AttitudeIndicator *attitude_indicator_new(int width, int height)
{

    AttitudeIndicator *self;
    self = calloc(1, sizeof(AttitudeIndicator));
    if(self){
        if(!attitude_indicator_init(self, width, height)){
            free(self);
            return NULL;
        }
    }
    return self;
}


AttitudeIndicator *attitude_indicator_init(AttitudeIndicator *self, int width, int height)
{
    base_gauge_init(BASE_GAUGE(self), &attitude_indicator_ops, width, height);

	self->common_center.x = round((base_gauge_w(BASE_GAUGE(self)))/2.0);
#if 1
	self->common_center.y = round(base_gauge_h(BASE_GAUGE(self))*0.4);
#else
	self->common_center.y = round(base_gauge_h(BASE_GAUGE(self))/2.0);
#endif
	self->size = 2; /*In tens of degrees, here 20deg (+/-)*/

    /*TODO: Failure*/
    generic_layer_init_from_file(&self->markers[MARKER_LEFT], "left-marker.png");
    generic_layer_init_from_file(&self->markers[MARKER_RIGHT], "right-marker.png");
    generic_layer_init_from_file(&self->markers[MARKER_CENTER], "center-marker.png");
    for(int i = 0; i < 3; i++)
        generic_layer_build_texture(&self->markers[i]);

	self->rollslip = roll_slip_gauge_new();
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->rollslip),
        self->locations[ROLL_SLIP].x,
        self->locations[ROLL_SLIP].y
    );

	self->locations[MARKER_LEFT] = (SDL_Rect){
	/*The left marker has its arrow pointing right and the arrow X is at marker->w-1*/
		self->common_center.x - 78 - (generic_layer_w(&self->markers[0])-1),
		self->common_center.y - round(generic_layer_h(&self->markers[0])/2.0) /*+1*/,
		generic_layer_w(&self->markers[MARKER_LEFT]), generic_layer_h(&self->markers[MARKER_LEFT])
	};
	self->locations[MARKER_RIGHT] = (SDL_Rect){
		self->common_center.x + 78,
		self->locations[MARKER_LEFT].y,
		generic_layer_w(&self->markers[MARKER_RIGHT]), generic_layer_h(&self->markers[MARKER_RIGHT])
	};
	self->locations[MARKER_CENTER] = (SDL_Rect){
		self->common_center.x - round((generic_layer_w(&self->markers[2])-1)/2.0),
		self->common_center.y /*+ 1*/,
		generic_layer_w(&self->markers[MARKER_CENTER]), generic_layer_h(&self->markers[MARKER_CENTER])
	};

	self->locations[ROLL_SLIP] = (SDL_Rect){
		self->common_center.x - round((base_gauge_h(BASE_GAUGE(self->rollslip))-1)/2.0),
		7,
		0,0
	};
    base_gauge_add_child(BASE_GAUGE(self), BASE_GAUGE(self->rollslip),
        self->locations[ROLL_SLIP].x,
        self->locations[ROLL_SLIP].y
    );

    attitude_indicator_get_etched_ball(self);
#if ENABLE_3D
    /* Verticaly 7 pixels -> 1 degree, 2.5 degrees = 17 pixels
     * Horizontaly 8 pixels -> 1 degree.
     * Values found using screenshots and dividing
     * pixels by degrees. TODO: Find out how to
     * properly compute those values
     * */

    self->pitch_ruler.canvas = attitude_indicator_draw_ruler(self,
        self->size, 17,
        resource_manager_get_font(TERMINUS_12),
        &(SDL_Color){0,255,0}
    );
    /*TODO: Generate*/
    self->horizon_src = IMG_Load("horizon-grads-scaled.png");
    if(!self->horizon_src)
        return NULL;
    generic_layer_init(&self->etched_horizon, base_gauge_w(BASE_GAUGE(self)), self->horizon_src->h);
    generic_layer_build_texture(&self->pitch_ruler);
#endif

#if !USE_SDL_GPU
	self->state.rbuffer = SDL_CreateRGBSurfaceWithFormat(0, width*2, height*2, 32, SDL_PIXELFORMAT_RGBA32);
	self->renderer =  SDL_CreateSoftwareRenderer(self->state.rbuffer);
    self->ball_texture = SDL_CreateTextureFromSurface(self->renderer, self->etched_ball.canvas);
#endif

    return self;
}

static bool attitude_indicator_init_animations(AttitudeIndicator *self)
{
    /* We need to pre-create animations as there can/will be two
     * of them and we need to know which one is which*/
    BaseAnimation *animation;
    bool rv;

    animation = base_animation_new(TYPE_FLOAT, 2,
        &self->roll,
        &SFV_GAUGE(self->rollslip)->value
    );
    if(!animation)
        return false;
    rv = base_gauge_add_animation(BASE_GAUGE(self), animation);
    base_animation_unref(animation);/*base_gauge takes ownership*/
    if(!rv)
        return false;

    animation = base_animation_new(TYPE_FLOAT, 1, &self->pitch);
    if(!animation)
        return false;
    rv = base_gauge_add_animation(BASE_GAUGE(self), animation);
    base_animation_unref(animation);/*base_gauge takes ownership*/

    return rv;
}

void attitude_indicator_dispose(AttitudeIndicator *self)
{
	base_gauge_dispose(BASE_GAUGE(self));

	roll_slip_gauge_free(self->rollslip);
	for(int i = 0; i < 3; i++){
        generic_layer_dispose(&self->markers[i]);
	}
#if !USE_SDL_GPU
	if(self->state.rbuffer)
		SDL_FreeSurface(self->state.rbuffer);
	SDL_DestroyRenderer(self->renderer);
#endif
    generic_layer_dispose(&self->etched_ball);
#if ENABLE_3D
    generic_layer_dispose(&self->etched_horizon);
    if(self->horizon_src)
        SDL_FreeSurface(self->horizon_src);
#endif
}

void attitude_indicator_free(AttitudeIndicator *self)
{
	attitude_indicator_dispose(self);
	free(self);
}

bool attitude_indicator_set_roll(AttitudeIndicator *self, float value, bool animated)
{
    BaseAnimation *animation;
    bool rv;

    animated = (self->mode == AI_MODE_3D) ? false : animated;

    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            rv = attitude_indicator_init_animations(self);
            if(!rv)
                goto fallback;
        }
        animation = BASE_GAUGE(self)->animations[AI_ROLL_ANIMATION];
        base_animation_start(animation, self->roll, value, DEFAULT_DURATION);
    }else{
fallback:
        self->roll = value;
        roll_slip_gauge_set_value(self->rollslip, value, false);
        BASE_GAUGE(self)->dirty = true;
    }
    return true;
 }

bool attitude_indicator_set_pitch(AttitudeIndicator *self, float value, bool animated)
{
    BaseAnimation *animation;
    bool rv;

	value = (value > self->size*10) ? self->size*10 + 5 : value;
	value = (value < self->size*-10) ? self->size*-10 - 5: value;

    animated = (self->mode == AI_MODE_3D) ? false : animated;

//    printf("%s %p value: %f\n",__FUNCTION__, self, value);
    if(animated){
        if(BASE_GAUGE(self)->nanimations == 0){
            rv = attitude_indicator_init_animations(self);
            if(!rv)
                goto fallback;
        }
        animation = BASE_GAUGE(self)->animations[AI_PITCH_ANIMATION];
        base_animation_start(animation, self->pitch, value, DEFAULT_DURATION);
    }else{
fallback:
        if(value != self->pitch){
            self->pitch = value;
            BASE_GAUGE(self)->dirty = true;
        }
    }
    return true;
}

bool attitude_indicator_set_heading(AttitudeIndicator *self, float value)
{
    bool rv;

    if(value > 360 || value < 0) return false;

    self->heading = value;
    BASE_GAUGE(self)->dirty = true;

    return true;
}


Uint32 interpolate_color_p(const SDL_PixelFormat *format, Uint32 from, Uint32 to, float progress)
{
    Uint8 fr,fg,fb;
    Uint8 tr,tg,tb;
    Uint8 rv_r, rv_g, rv_b;

    SDL_GetRGB(from, format, &fr, &fg, &fb);
    SDL_GetRGB(to, format, &tr, &tg, &tb);
//	printf("Interpolate between (%d,%d,%d) and (%d,%d,%d), progress=%0.2f percent\n",fr,fg,fb,tr,tg,tb, progress*100);

#if 1
    rv_r = fr + round((tr-fr)*progress);
    rv_g = fg + round((tg-fg)*progress);
    rv_b = fb + round((tb-fb)*progress);
#elif 0
    rv_r = round((1.0-progress) * fr + progress * tr + 0.5);
    rv_g = round((1.0-progress) * fg + progress * tg + 0.5);
    rv_b = round((1.0-progress) * fb + progress * tb + 0.5);
#endif

    return SDL_MapRGB(format, rv_r, rv_g, rv_b);
}

float range_progress(float value, float start, float end)
{
	float rv;
	float _v, _s, _e;
	_v = value - start;
	_e = end - start;

	rv = _v/_e;

//    printf("Progress=%0.2f value=%0.2f start=%0.2f, end=%0.2f\n",rv, value, start, end);
	return rv;
}

SDL_Surface *attitude_indicator_draw_ball(AttitudeIndicator *self)
{
    Uint32 *pixels;
    SDL_Surface *surface;
    int x, y, limit;
    Uint32 white;
    Uint32 sky,sky_down;
    Uint32 earth;

    self->ball_window = (SDL_Rect){
        .x = 0, .y = 0,
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = base_gauge_h(BASE_GAUGE(self))
    };

    self->ball_all = (SDL_Rect){
        .x = 0, .y = 0,
        .w = self->ball_window.w*2,
        .h = self->ball_window.h*2
    };

    surface = SDL_CreateRGBSurfaceWithFormat(0, self->ball_all.w, self->ball_all.h, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_LockSurface(surface);
    pixels = surface->pixels;

    white = SDL_UWHITE(surface);
//    earth = SDL_MapRGB(surface->format, 0x46, 0x2b, 0x0c);
//    earth = SDL_MapRGB(surface->format, 0x45, 0x2a, 0x06);
//    earth = SDL_MapRGB(surface->format, 0x44, 0x28, 0x07);
//    earth = SDL_MapRGB(surface->format, 0x3f, 0x28, 0x0a);
    earth = SDL_MapRGB(surface->format, 0x58, 0x34, 0x0a);
//    sky = SDL_MapRGB(surface->format, 0x13, 0x51, 0xd3);
    sky = SDL_MapRGB(surface->format, 0x00, 0x50, 0xff);
    sky_down = SDL_MapRGB(surface->format, 0x52, 0x6c, 0xd0);



    //limit = round(surface->h*0.4); /*40/60 split between sky and earth*/
    SDLExt_RectCenter(&self->ball_window, &self->ball_all); /*window x,y coordinates are now relative to "all" x,y*/
    limit = self->ball_window.y + round(self->ball_window.h*0.4); /*40/60 split between sky and earth*/

	self->ball_horizon = limit; /*self->ball_horizon is in "all" units*/
    self->ball_center.y = limit; /*TODO: Merge these two center.y and limit*/
    self->ball_center.x = round(self->ball_all.w/2.0)-1;

	int first_gradient = round(limit * 0.25); /*First gradiant from the centerline to 25% up*/
	int first_gradiant_stop = limit - first_gradient;
	Uint32 color;

    /*Draw the sky*/
    for(y = limit; y >= 0; y--){
		if(y  > first_gradiant_stop){
			float progress;
			progress = range_progress(y, limit, first_gradiant_stop);

			color = interpolate_color_p(surface->format, sky_down, sky, progress );
		}else{
			color = sky;
//			exit(0);
		}
        for(int x = 0; x < surface->w; x++){
	        pixels[y * surface->w + x] = color;
        }
    }

    /*Draw a white center line*/
	y = limit;
    for(x = 0; x < surface->w; x++){
        pixels[y * surface->w + x] = white;
    }

    /*Draw the earth*/
    int start = y +1;
    for(y = start; y < surface->h; y++){
        for(int x = 0; x < surface->w; x++){
//            pixels[y * surface->w + x] = interpolate_color(surface->format, white, earth, y, start, surface->h-1);
            pixels[y * surface->w + x] = earth;
        }
    }
    SDL_UnlockSurface(surface);

    return surface;
}

/**
 * Draws a vertical pitch "ruler".
 *
 * Calling code must free the returned surface.
 * Internal use only
 *
 * TODO: Replace this with GenericRuler
 *
 * @param self a AttitudeIndicator
 * @param size The number of 10th graduations, both ways. A value of 2 means
 * etches from 20 to -20 degrees.
 * @param ppm Pixels per 2.5 etch. How much pixel a 2.5 degree interval
 * (base etching) does take.
 * @param font the PCF_Font used to draw the marks
 */
static SDL_Surface *attitude_indicator_draw_ruler(AttitudeIndicator *self, int size, int ppm, PCF_Font *font, SDL_Color *col)
{
    SDL_Surface *rv;
	Uint32 *pixels;
	int width, height;
	int x, y;
	int start_x, middle_x, end_x;
	int middle_y;

//    int ppm = 9; /*17*/ /*pixels per mark*/
    int hfactor = (20/2.5)*ppm + 1; /*20 is the span between -10 to 10*/
	int yoffset = 10;

	/* Width: 57px wide for the 10s graduations plus 2x20 to allow space for the font*/
	width = 57+(2*20);
	middle_x = (57-1)/2 + 19;
	 /* Height: hfactor px for +10 and -10 graduations by the size */
	height = hfactor*size + 20;
	printf("max height: %d\n", height-1); //166 values from 0 to 165
	middle_y = ((hfactor*size)-1)/2.0 + yoffset;

	self->ruler_center.x = middle_x;
	self->ruler_center.y = middle_y;

	rv = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
//	SDL_SetColorKey(rv, SDL_TRUE, SDL_UCKEY(rv));
//	SDL_FillRect(rv, NULL, SDL_UCKEY(rv));


    SDL_LockSurface(rv);
    pixels = rv->pixels;
    Uint32 color = SDL_MapRGB(rv->format, col->r, col->g, col->b);
    Uint32 mcolor = SDL_URED(rv);

	int grad_level;
	int grad_sizes[] = {57,11,25,11};

	/*Go upwards*/
	grad_level = 0;
	for(y = middle_y; y >= 0+yoffset; y--){
		if( (y-middle_y) % ppm == 0){
			start_x = middle_x - (grad_sizes[grad_level]-1)/2;
			end_x = middle_x + (grad_sizes[grad_level]-1)/2;

			for( x = start_x; x <= end_x; x++){
                pixels[y * rv->w + x] = color;
			}
            pixels[y * rv->w + middle_x] = mcolor;

			grad_level = (grad_level < 3) ? grad_level + 1 : 0;
		}
	}
	/*Go downwards*/
	grad_level = 0;
	for(y = middle_y; y < rv->h-yoffset; y++){
		if( (y-middle_y) % ppm == 0){
			start_x = middle_x - (grad_sizes[grad_level]-1)/2;
			end_x = middle_x + (grad_sizes[grad_level]-1)/2;

			for( x = start_x; x <= end_x; x++){
                pixels[y * rv->w + x] = color;
			}
            pixels[y * rv->w + middle_x] = mcolor;

			grad_level = (grad_level < 3) ? grad_level + 1 : 0;
		}
	}
    SDL_UnlockSurface(rv);

    int current_grad;
    Uint32 tcol; /*text color*/

    tcol = SDL_MapRGB(rv->format, col->r, col->g, col->b);

	/*Go upwards*/
	current_grad = 0;
	for(y = middle_y; y >= 0+yoffset; y--){
		if((y-middle_y) % (4*ppm) == 0){ // 10 graduation
			if(current_grad > 0){
                PCF_FontWriteNumberAt(font,
                    &current_grad, TypeInt, 2,
                    tcol, rv,
                    middle_x - (57-1)/2 - 4, y, LeftToCol | CenterOnRow
                );
                PCF_FontWriteNumberAt(font,
                    &current_grad, TypeInt, 2,
                    tcol, rv,
                    middle_x + (57-1)/2 + 4, y, RightToCol | CenterOnRow
                );
			}
			current_grad += 10;
		}
	}

	/*Go downwards*/
	current_grad = 0;
	for(y = middle_y; y < rv->h-yoffset; y++){
		if((y-middle_y) % (4*ppm) == 0){ // 10 graduation
			if(current_grad > 0){
                PCF_FontWriteNumberAt(font,
                    &current_grad, TypeInt, 2,
                    tcol, rv,
                    middle_x - (57-1)/2 - 4, y, LeftToCol | CenterOnRow
                );
                PCF_FontWriteNumberAt(font,
                    &current_grad, TypeInt, 2,
                    tcol, rv,
                    middle_x + (57-1)/2 + 4, y, RightToCol | CenterOnRow
                );
			}
			current_grad += 10;
		}
	}

    return rv;
}


/*TODO: This might go in a Ruler class*/
int attitude_indicator_resolve_value(AttitudeIndicator *self, float value)
{
	float aval;
	float ngrads;

	aval = fabs(value);
	if(aval == 0){
		ngrads = 0;
	}else if(!fmod(aval, 2.5)){
		ngrads = aval/2.5;
	}else if(!fmod(aval, 5.0)){
		ngrads = aval/5;
	}else if(!fmod(aval, 10)){
		ngrads = aval/10;
	}else {
		ngrads = aval/2.5;
	}

	return self->common_center.y + sign(value) * round((ngrads * 9.0));
}

/*TODO: This might go in a Ruler class*/

/* Returns a y increment from the position where the ball horizon is aligned with
 * the view "rubis".
 * */
int attitude_indicator_resolve_increment(AttitudeIndicator *self, float value)
{
	float aval;
	float ngrads;

	aval = fabs(value);
	if(aval == 0){
		ngrads = 0;
	}else if(!fmod(aval, 2.5)){
		ngrads = aval/2.5;
	}else if(!fmod(aval, 5.0)){
		ngrads = aval/5;
	}else if(!fmod(aval, 10)){
		ngrads = aval/10;
	}else {
		ngrads = aval/2.5;
	}

	return sign(value) * round((ngrads * 9.0));
}



static SDL_Surface *attitude_indicator_get_etched_ball(AttitudeIndicator *self)
{
	if(!self->etched_ball.canvas){
        SDL_Surface *ball, *ruler;
		SDL_Rect ball_pos;
		SDL_Rect ruler_pos;

        ball = attitude_indicator_draw_ball(self);
        ruler = attitude_indicator_draw_ruler(self, self->size, 9, resource_manager_get_font(TERMINUS_12), &SDL_WHITE);

        generic_layer_init(&self->etched_ball, self->ball_all.w, self->ball_all.h);

		/* First place the ball and the scale, such has the middle of the the scale is on
		 * the same line as the "middle" of the ball. They both need to have the same y coordinate
		 * on screen.
		 * */
		SDL_BlitSurface(ball, NULL, self->etched_ball.canvas, NULL);

		ruler_pos.x = round(self->ball_all.w/2.0) - self->ruler_center.x;
		ruler_pos.y = self->ball_horizon - self->ruler_center.y;

		SDL_BlitSurface(ruler, NULL, self->etched_ball.canvas, &ruler_pos);

        SDL_FreeSurface(ball);
        SDL_FreeSurface(ruler);
	}
    generic_layer_build_texture(&self->etched_ball);

	return self->etched_ball.canvas;
}

static void attitude_indicator_update_state(AttitudeIndicator *self, Uint32 dt)
{
    BaseAnimation *animation;

    if(BASE_GAUGE(self)->nanimations > 0){
        animation = BASE_GAUGE(self)->animations[0];
        if(!animation->finished){
            BASE_GAUGE(self->rollslip)->dirty = true;
        }
    }

    /*First find out a view-sized window into the larger ball buffer for a 0deg pitch*/
    self->state.win = (SDL_Rect){
        .x = self->ball_center.x - (round(base_gauge_w(BASE_GAUGE(self))/2.0) -1), /*Is ball_window useless?*/
        .y = self->ball_center.y - (self->common_center.y - 1),
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = base_gauge_h(BASE_GAUGE(self))
    };
    /*Then apply to correct y offset to account for pitch*/
    self->state.win.y += attitude_indicator_resolve_increment(self, self->pitch * -1.0);

    /* Now that we have the correct self->state.window, rotate it to account for roll
     * The rotation center must be 40% of the height and middle width
     * */
    self->state.rcenter = (SDL_Point){
        .x = self->state.win.x + (round(self->state.win.w/2.0)-1),
        .y = self->state.win.y + (round(self->state.win.h*0.4)-1)
    };

    self->state.dst_clip = (SDL_Rect){
        self->state.rcenter.x - self->state.win.x,
        self->state.rcenter.y - self->state.win.y,
        0,0
    };
#if !USE_SDL_GPU
	if(self->roll != 0){
        SDL_RenderCopyEx(self->renderer, self->ball_texture,
            NULL, NULL,
            self->roll, &self->state.rcenter,
            SDL_FLIP_NONE
        );
	}else{
        SDL_BlitSurface(self->etched_ball.canvas, NULL, self->state.rbuffer, NULL);
    }
#endif
#if ENABLE_3D
    int horizon_y = self->common_center.y-1;
    int increment = 0;
    increment = self->pitch * 7; /*7 pixels 1 degree*/
    horizon_y += increment;

    /*TODO: This Whole infinite horizon handling code
     * should go in a common infinite/looping ruler handler
     */
    /*Resolve the heading value in a pixel x-coordinate in the image*/
    int value_x = 0;
    if(self->heading >= 0 && self->heading <= 230){
        value_x = 1024 + self->heading / (1.0/8.0); /*degrees per pixel*/
    }else if(self->heading >= 232 && self->heading <= 359){
        float tmp = self->heading - 232;
        value_x = 0 + tmp / (1.0/8.0);
    }

    /*
     * Postionning / centering to have
     * hz_srect.x + rubis = value_x
     */
    int rubis = self->common_center.x;
    int xbegin = value_x - rubis;
    int xend;
    bool rpatch;
    int xcursor = 0;
    SDL_Rect hz_srects[3];
    SDL_Rect hz_drects[3];
    int npatches = 0;

    if(xbegin < 0){ /* we went before the image begining and need to fill the left side*/
        hz_srects[npatches].x = self->horizon_src->w-1 - abs(xbegin);
        hz_srects[npatches].w = abs(xbegin);
        hz_srects[npatches].y = 0;
        hz_srects[npatches].h = self->horizon_src->h;

        hz_drects[npatches].x = xcursor;
        hz_drects[npatches].w = hz_srects[npatches].w;
        //hz_drects[npatches].y = horizon_y - self->horizon_src->h;
        hz_drects[npatches].y = 0;
        hz_drects[npatches].h = self->horizon_src->h;

        xcursor += hz_drects[npatches].w;
        xbegin = 0;
        npatches++;
    }
    hz_srects[npatches].x = xbegin;
    hz_srects[npatches].w = base_gauge_w(BASE_GAUGE(self));
    hz_srects[npatches].y = 0;
    hz_srects[npatches].h = self->horizon_src->h;

    xend = hz_srects[npatches].x + hz_srects[npatches].w;
    if(xend > self->horizon_src->w-1){
        hz_srects[npatches].w = self->horizon_src->w - hz_srects[npatches].x;
    }

    hz_drects[npatches].x = xcursor;
    hz_drects[npatches].w = hz_srects[npatches].w;
    hz_drects[npatches].y = 0;
    hz_drects[npatches].h = self->horizon_src->h;

    xcursor += hz_drects[npatches].w;
    npatches++;

    if(xend > self->horizon_src->w-1){
        int pixels = xend - self->horizon_src->w-1;
        hz_srects[npatches].x = 0;
        hz_srects[npatches].w = pixels;
        hz_srects[npatches].y = 0;
        hz_srects[npatches].h = self->horizon_src->h;

        hz_drects[npatches].x = xcursor;
        hz_drects[npatches].w = hz_srects[npatches].w;
        hz_drects[npatches].y = 0;
        hz_drects[npatches].h = self->horizon_src->h;

        npatches++;
    }

    SDL_FillRect(self->etched_horizon.canvas, NULL, 0x00000000);
    for(int i = 0; i < npatches; i++){
        SDL_BlitSurface(
            self->horizon_src, &hz_srects[i],
            self->etched_horizon.canvas, &hz_drects[i]
        );
    }
    generic_layer_update_texture(&self->etched_horizon);

    self->state.hz_drect = (SDL_Rect){
        .x = 0,
        .y = horizon_y - self->horizon_src->h,
        .w = base_gauge_w(BASE_GAUGE(self)),
        .h = self->horizon_src->h
    };

    /*Pitch ruler*/
    self->state.pr_dstrect = (SDL_Rect) {
        .x = base_gauge_w(BASE_GAUGE(self))/2 - self->pitch_ruler.texture->w/2 + 1,
        .y = horizon_y - (self->pitch_ruler.texture->h/2-1),
        .w = self->pitch_ruler.texture->w,
        .h = self->pitch_ruler.texture->h
    };

    self->state.rcenter_3d.x = self->pitch_ruler.texture->w/2;
    self->state.rcenter_3d.y = self->pitch_ruler.texture->h/2 - increment;

    self->state.hz_rcenter.x = self->etched_horizon.texture->w/2;
    self->state.hz_rcenter.y = self->etched_horizon.texture->h/2 - increment;

#endif
}

static void attitude_indicator_render(AttitudeIndicator *self, Uint32 dt, RenderContext *ctx)
{
#if USE_SDL_GPU
    if(self->mode == AI_MODE_2D){
        base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
            self->etched_ball.texture,
            NULL,
            -self->roll,
            &self->state.rcenter,
            NULL,
            &self->state.dst_clip);
    }else{
#if ENABLE_3D
        base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
            self->etched_horizon.texture,
            NULL,
            -self->roll,
            &self->state.hz_rcenter,
            &self->state.hz_drect,
            NULL
        );

        base_gauge_blit_rotated_texture(BASE_GAUGE(self), ctx,
         self->pitch_ruler.texture,
         NULL,
         -self->roll,
         &self->state.rcenter_3d,
         &self->state.pr_dstrect,
         NULL);
#endif
    }
#else
    base_gauge_blit(BASE_GAUGE(self), ctx,
        self->state.rbuffer,
        &self->state.win,
        NULL
    );
#endif
    base_gauge_blit_layer(BASE_GAUGE(self), ctx, &self->markers[MARKER_LEFT], NULL, &self->locations[MARKER_LEFT]);
    base_gauge_blit_layer(BASE_GAUGE(self), ctx, &self->markers[MARKER_RIGHT], NULL, &self->locations[MARKER_RIGHT]);
    base_gauge_blit_layer(BASE_GAUGE(self), ctx, &self->markers[MARKER_CENTER], NULL, &self->locations[MARKER_CENTER]);
}
