#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "animated-gauge.h"
#include "attitude-indicator.h"
#include "base-gauge.h"
#include "buffered-gauge.h"
#include "misc.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "sdl-pcf/SDL_pcf.h"

#define sign(x) (((x) > 0) - ((x) < 0))

static void attitude_indicator_render_value(AttitudeIndicator *self, float value);
static SDL_Surface *attitude_indicator_get_etched_ball(AttitudeIndicator *self);
static AnimatedGaugeOps attitude_indicator_ops = {
   .render_value = (ValueRenderFunc)attitude_indicator_render_value
};



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
    animated_gauge_init(ANIMATED_GAUGE(self), ANIMATED_GAUGE_OPS(&attitude_indicator_ops), width, height);

	self->common_center.x = round((BASE_GAUGE(self)->w)/2.0);
	self->common_center.y = round(BASE_GAUGE(self)->h*0.4);
	self->size = 2; /*In tens of degrees, here 20deg (+/-)*/

	self->markers[MARKER_LEFT] = IMG_Load("left-marker.png");
	self->markers[MARKER_RIGHT] = IMG_Load("right-marker.png");
	self->markers[MARKER_CENTER] = IMG_Load("center-marker.png");

	self->rollslip = roll_slip_gauge_new();

	self->locations[MARKER_LEFT] = (SDL_Rect){
	/*The left marker has its arrow pointing right and the arrow X is at marker->w-1*/
		self->common_center.x - 78 - (self->markers[0]->w-1),
		round(BASE_GAUGE(self)->h*0.4) - round(self->markers[0]->h/2.0) +1,
		0,0
	};
	self->locations[MARKER_RIGHT] = (SDL_Rect){
		self->common_center.x + 78,
		self->locations[MARKER_LEFT].y,
		0,0
	};
	self->locations[MARKER_CENTER] = (SDL_Rect){
		self->common_center.x - round((self->markers[2]->w-1)/2.0),
		round(BASE_GAUGE(self)->h*0.4) +1,
		0,0
	};

	self->locations[ROLL_SLIP] = (SDL_Rect){
		self->common_center.x - round((BASE_GAUGE(self->rollslip)->w-1)/2.0),
		7,
		0,0
	};

	self->buffer = SDL_CreateRGBSurfaceWithFormat(0, width*2, height*2, 32, SDL_PIXELFORMAT_RGBA32);
	self->renderer =  SDL_CreateSoftwareRenderer(self->buffer);

    attitude_indicator_get_etched_ball(self);
    return self;
}

void attitude_indicator_dispose(AttitudeIndicator *self)
{
	animated_gauge_dispose(ANIMATED_GAUGE(self));

	roll_slip_gauge_free(self->rollslip);
	for(int i = 0; i < 3; i++){
		if(self->markers[i])
			SDL_FreeSurface(self->markers[i]);
	}
	if(self->buffer)
		SDL_FreeSurface(self->buffer);
	if(self->etched_ball)
		SDL_FreeSurface(self->etched_ball);
	SDL_DestroyRenderer(self->renderer);
}

void attitude_indicator_free(AttitudeIndicator *self)
{
	attitude_indicator_dispose(self);
	free(self);
}

void attitude_indicator_set_roll(AttitudeIndicator *self, float value)
{
	animated_gauge_set_value(ANIMATED_GAUGE(self->rollslip), value);
	BUFFERED_GAUGE(self)->damaged = true;
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
        .w = BASE_GAUGE(self)->w,
        .h = BASE_GAUGE(self)->h
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

SDL_Surface *attitude_indicator_draw_ruler(AttitudeIndicator *self, int size, PCF_Font *font)
{
    SDL_Surface *rv;
	Uint32 *pixels;
	int width, height;
	int x, y;
	int start_x, middle_x, end_x;
	int middle_y;

	int yoffset = 10;
	/* Width: 57px wide for the 10s graduations plus 2x20 to allow space for the font*/
	width = 57+(2*20);
	middle_x = (57-1)/2 + 19;
	 /* Height: 73px for +10 and -10 graduations by the size */
	height = 73*size + 20;
	printf("max height: %d\n", height-1); //166 values from 0 to 165
	middle_y = ((73*size)-1)/2.0 + yoffset;

	self->ruler_center.x = middle_x;
	self->ruler_center.y = middle_y;

	rv = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	SDL_SetColorKey(rv, SDL_TRUE, SDL_UCKEY(rv));
	SDL_FillRect(rv, NULL, SDL_UCKEY(rv));


    SDL_LockSurface(rv);
    pixels = rv->pixels;
    Uint32 color = SDL_UWHITE(rv);
    Uint32 mcolor = SDL_URED(rv);

	int grad_level;
	int grad_sizes[] = {57,11,25,11};

	/*Go upwards*/
	grad_level = 0;
	for(y = middle_y; y >= 0+yoffset; y--){
		if( (y-middle_y) % 9 == 0){
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
		if( (y-middle_y) % 9 == 0){
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

	char number[4]; /*Three digits plus \0*/
	uintf8_t current_grad;
	SDL_Rect left, right;
    Uint32 text_w;
    Uint32 tcol;

    tcol = SDL_UWHITE(rv);

	start_x = middle_x - (57-1)/2;
	end_x = middle_x + (57-1)/2;

    int empty_top_pix = font->xfont.fontPrivate->metrics->metrics.ascent -  font->xfont.fontPrivate->ink_metrics->ascent;
    int glyph_middle =  empty_top_pix + round(font->xfont.fontPrivate->ink_metrics->ascent/2.0);


	/*Go upwards*/
	current_grad = 0;
	for(y = middle_y; y >= 0+yoffset; y--){
		if((y-middle_y) % 36 == 0){ // 10 graduation
			if(current_grad > 0){
				snprintf(number, 4, "%d", current_grad);

                PCF_FontGetSizeRequest(font, number, &text_w, NULL);

				left.x = middle_x - (57-1)/2 - (text_w + 4);
				right.x = middle_x + (57-1)/2 + (4);
				left.y = y - glyph_middle;
				right.y = left.y;
                PCF_FontWrite(font, number, tcol, rv, &left);
                PCF_FontWrite(font, number, tcol, rv, &right);
			}

			current_grad += 10;
		}
	}

	/*Go downwards*/
	current_grad = 0;
	for(y = middle_y; y < rv->h-yoffset; y++){
		if((y-middle_y) % 36 == 0){ // 10 graduation
			if(current_grad > 0){
				snprintf(number, 4, "%d", current_grad);

                PCF_FontGetSizeRequest(font, number, &text_w, NULL);

				left.x = middle_x - (57-1)/2 - (text_w + 4);
				right.x = middle_x + (57-1)/2 + (4);
				left.y = y - glyph_middle;
				right.y = left.y;

                PCF_FontWrite(font, number, tcol, rv, &left);
                PCF_FontWrite(font, number, tcol, rv, &right);
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
	if(!self->etched_ball){
        SDL_Surface *ball, *ruler;
		SDL_Rect ball_pos;
		SDL_Rect ruler_pos;

        ball = attitude_indicator_draw_ball(self);
        ruler = attitude_indicator_draw_ruler(self, self->size, resource_manager_get_font(TERMINUS_12));

		self->etched_ball = SDL_CreateRGBSurfaceWithFormat(0, self->ball_all.w, self->ball_all.h, 32, SDL_PIXELFORMAT_RGBA32);

		//int common_y = attitude_indicator_resolve_value(self, value);
		int common_y = attitude_indicator_resolve_value(self, 0);

		/* First place the ball and the scale, such has the middle of the the scale is on
		 * the same line as the "middle" of the ball. They both need to have the same y coordinate
		 * on screen.
		 * */
		SDL_BlitSurface(ball, NULL, self->etched_ball, NULL);

		ruler_pos.x = round(self->ball_all.w/2.0) - self->ruler_center.x;
		ruler_pos.y = self->ball_horizon - self->ruler_center.y;
		SDL_BlitSurface(ruler, NULL, self->etched_ball, &ruler_pos);

        SDL_FreeSurface(ball);
        SDL_FreeSurface(ruler);
	}

	return self->etched_ball;
}

static void attitude_indicator_render_value(AttitudeIndicator *self, float value)
{
	SDL_Surface *surface;
	SDL_Rect ball_pos;


//	printf("Attitude indicator rendering value %0.2f\n",value);

	value = (value > self->size*10) ? self->size*10 + 5 : value;
	value = (value < self->size*-10) ? self->size*-10 - 5: value;

    value = value * -1.0;
    buffered_gauge_clear(BUFFERED_GAUGE(self), NULL);


    /*First find out a view-sized window into the larger ball buffer for a 0deg pitch*/
    SDL_Rect win = {
        .x = self->ball_center.x - (round(BASE_GAUGE(self)->w/2.0) -1), /*Is ball_window useless?*/
        .y = self->ball_center.y - (round(BASE_GAUGE(self)->h*0.4) -1),
        .w = BASE_GAUGE(self)->w,
        .h = BASE_GAUGE(self)->h
    };
    /*Then apply to correct y offset to account for pitch*/
    win.y += attitude_indicator_resolve_increment(self, value);
    /* Now that we have the correct window, rotate it to account for roll
     * The rotation center must be 40% of the height and middle width
     * */
    SDL_Point rcenter = {
        .x = win.x + (round(win.w/2.0)-1),
        .y = win.y + (round(win.h*0.4)-1)
    };
    surface = attitude_indicator_get_etched_ball(self); /*TODO: Change to etched ball*/
	if(self->rollslip->super.value != 0){
        SDL_Texture *tex = SDL_CreateTextureFromSurface(self->renderer, surface);
        SDL_RenderCopyEx(self->renderer, tex, NULL, NULL, self->rollslip->super.value, &rcenter, SDL_FLIP_NONE);
		SDL_DestroyTexture(tex);
        surface = self->buffer;
	}
    buffered_gauge_blit(BUFFERED_GAUGE(self), surface, &win, NULL);

	/*Place the roll indicator*/
    /* Temp fix, the function triggering the rendering of AttitudeIndicator must also manually render self->rollslip
     * as it needs to animate its value which is not known here.
     *
     * TODO: Either compose AttitudeIndicator with the two gauges artifical horizon and rollslip gauge, or
     * extract rollslip from attitudeindicator, or something else
     * */
//	SDL_BlitSurface(self->rollslip->super.view, NULL, tview, &self->locations[ROLL_SLIP]);

	/*Then place markers in the middle of the *screen* markers don't move*/
    buffered_gauge_blit(BUFFERED_GAUGE(self), self->markers[MARKER_LEFT], NULL, &self->locations[MARKER_LEFT]);
    buffered_gauge_blit(BUFFERED_GAUGE(self), self->markers[MARKER_RIGHT], NULL, &self->locations[MARKER_RIGHT]);
    buffered_gauge_blit(BUFFERED_GAUGE(self), self->markers[MARKER_CENTER], NULL, &self->locations[MARKER_CENTER]);
}
