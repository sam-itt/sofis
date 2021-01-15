#ifndef ATTITUDE_INDICATOR_H
#define ATTITUDE_INDICATOR_H

#include "base-gauge.h"
#include "generic-layer.h"
#include "roll-slip-gauge.h"

#define MARKER_LEFT 0
#define MARKER_RIGHT 1
#define MARKER_CENTER 2
#define ROLL_SLIP 3
#define LOCATION_MAX 5

typedef enum{
    AI_ROLL_ANIMATION,
    AI_PITCH_ANIMATION,
    N_AI_ANIMATIONS
}AIttitudeIndicatorAnimation;

typedef enum{
    AI_MODE_2D = 0,
    AI_MODE_3D
}AttitudeIndicatorDisplayMode;

typedef struct{
    SDL_Point rcenter;
    SDL_Rect dst_clip;
    SDL_Rect win;
#if !USE_SDL_GPU
    SDL_Surface *rbuffer; /*rotation buffer*/
#endif
#if ENABLE_3D
    SDL_Rect hz_srects[3];
    SDL_Rect hz_drects[3];
    int npatches;

    SDL_Rect pr_dstrect;
#endif
}AttitudeIndicatorState;

typedef struct{
    BaseGauge super;
	SDL_Point common_center;

    float roll; /*pitch?*/
    float pitch;
    float heading;

	RollSlipGauge *rollslip;

	int size; /*number of 10s markings*/
    AttitudeIndicatorDisplayMode mode;

    SDL_Rect ball_window; /*Visible portion*/
    SDL_Rect ball_all; /*Visible portion plus extended area*/
	int ball_horizon;
    SDL_Point ball_center;

	SDL_Point ruler_center;
	int ruler_middle;
	int ruler_middlex;

    GenericLayer markers[3]; //left, right, center
	SDL_Rect locations[LOCATION_MAX];
#if !USE_SDL_GPU
	SDL_Renderer *renderer;
	SDL_Texture *ball_texture;
#endif

    GenericLayer etched_ball;
#if USE_SDL_GPU && ENABLE_3D
    GenericLayer pitch_ruler;
    GenericLayer etched_horizon;
#endif

    AttitudeIndicatorState state;
}AttitudeIndicator;


AttitudeIndicator *attitude_indicator_new(int width, int height);
AttitudeIndicator *attitude_indicator_init(AttitudeIndicator *self, int width, int height);
void attitude_indicator_dispose(AttitudeIndicator *self);
void attitude_indicator_free(AttitudeIndicator *self);

bool attitude_indicator_set_roll(AttitudeIndicator *self, float value, bool animated);
bool attitude_indicator_set_pitch(AttitudeIndicator *self, float value, bool animated);
bool attitude_indicator_set_heading(AttitudeIndicator *self, float value);

AttitudeIndicator *attitude_indicator_init(AttitudeIndicator *self, int width, int height);

#endif /* ATTITUDE_INDICATOR_H */
