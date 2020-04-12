#ifndef ATTITUDE_INDICATOR_H
#define ATTITUDE_INDICATOR_H

#include "SDL_rect.h"
#include "SDL_surface.h"
#include "animated-gauge.h"

#define MARKER_LEFT 0
#define MARKER_RIGHT 1
#define MARKER_CENTER 2

typedef struct{
	int x,y;
}Point2D;

typedef struct{
    AnimatedGauge parent;
	Point2D common_center;

	int size;

	SDL_Surface *ball;
	int ball_horizon;

    SDL_Surface *ruler;
	Point2D ruler_center;
	int ruler_middle;
	int ruler_middlex;
//	int size;

	SDL_Surface *markers[3]; //left, right, center
	SDL_Rect locations[3];

}AttitudeIndicator;


AttitudeIndicator *attitude_indicator_new(int width, int height);
AttitudeIndicator *attitude_indicator_init(AttitudeIndicator *self, int width, int height);

#endif /* ATTITUDE_INDICATOR_H */
