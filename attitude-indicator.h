#ifndef ATTITUDE_INDICATOR_H
#define ATTITUDE_INDICATOR_H

#include "animated-gauge.h"
#include "roll-slip-gauge.h"

#define MARKER_LEFT 0
#define MARKER_RIGHT 1
#define MARKER_CENTER 2
#define ROLL_SLIP 3
#define LOCATION_MAX 5

typedef struct{
	int x,y;
}Point2D;

typedef struct{
    AnimatedGauge parent;
	Point2D common_center;

	RollSlipGauge *rollslip;

	int size; /*number of 10s markings*/

	SDL_Surface *ball;
	int ball_horizon;

    SDL_Surface *ruler;
	Point2D ruler_center;
	int ruler_middle;
	int ruler_middlex;
//	int size;

	SDL_Surface *markers[3]; //left, right, center
	SDL_Rect locations[LOCATION_MAX];

	SDL_Surface *buffer;
	SDL_Renderer *renderer;
	SDL_Texture *horizon;
	SDL_Surface *tmp_hz; /*To be deleted and replaced by the above texture*/


    SDL_Surface *ball_buffer;

}AttitudeIndicator;


AttitudeIndicator *attitude_indicator_new(int width, int height);
AttitudeIndicator *attitude_indicator_init(AttitudeIndicator *self, int width, int height);
void attitude_indicator_dispose(AttitudeIndicator *self);
void attitude_indicator_free(AttitudeIndicator *self);

void attitude_indicator_set_roll(AttitudeIndicator *self, float value);

AttitudeIndicator *attitude_indicator_init(AttitudeIndicator *self, int width, int height);

#endif /* ATTITUDE_INDICATOR_H */
