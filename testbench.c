#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "animated-gauge.h"
#include "basic-hud.h"
#include "ladder-gauge.h"
#include "alt-ladder-page-descriptor.h"

#include "odo-gauge.h"
#include "alt-indicator.h"
#include "vertical-stair.h"
#include "alt-group.h"
#include "airspeed-indicator.h"
#include "attitude-indicator.h"

#include "roll-slip-gauge.h"

#include "sdl-colors.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define N_COLORS 4

BasicHud *hud = NULL;
OdoGauge *gauge = NULL;
LadderGauge *ladder = NULL;
OdoGauge *wheel = NULL;
OdoGauge *odo = NULL;
AltIndicator *alt_ind = NULL;
VerticalStair *stair = NULL;
AltGroup *group = NULL;
AirspeedIndicator *asi = NULL;
AttitudeIndicator *ai = NULL;
RollSlipGauge *rsg = NULL;


float gval = 0.0;
//float alt = 1150.0;
float alt = 900.0;
float odo_val = 0.0;
//float vs = 2000.0;
float vs = 0.0;
float ias = 10.0;
//float pitch = 2.19;
float pitch = 0.0;
float roll = 0.0;
#define ODO_INC 1
#define VARIO_INC 100
#define IAS_INC 1
#define ALT_INC 150
#define PITCH_INC 1;
#define ROLL_INC 1;
#define GVAL_INC 10.0;

float compute_vs(float old_alt, float new_alt, Uint32 elapsed);

/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            if(event->state == SDL_PRESSED)
                return true;
            break;
        case SDLK_t:
            if(event->state == SDL_PRESSED){
                gval += GVAL_INC;
                if(gval > 99)
                    gval = 99;
                odo_gauge_set_value(gauge, gval);
            }
            break;
        case SDLK_g:
            if(event->state == SDL_PRESSED){
                gval -= GVAL_INC;
                if(gval < 0)
                    gval = 0;
                odo_gauge_set_value(gauge, gval);
            }
            break;
        case SDLK_a:
            if(event->state == SDL_PRESSED){
                ias += IAS_INC;
                airspeed_indicator_set_value(asi, ias);
                basic_hud_set(hud, 1, AIRSPEED, ias);
            }
            break;
        case SDLK_z:
            if(event->state == SDL_PRESSED){
                ias -= IAS_INC;
                airspeed_indicator_set_value(asi, ias);
                basic_hud_set(hud, 1, AIRSPEED, ias);
            }
            break;
        case SDLK_UP:
            if(event->state == SDL_PRESSED){
                vs = compute_vs(alt, alt+ALT_INC, elapsed);
                alt += ALT_INC;
                animated_gauge_set_value(ANIMATED_GAUGE(ladder), alt);
                odo_gauge_set_value(wheel, alt);
                alt_indicator_set_value(alt_ind, alt);
                //alt_group_set_altitude(group, alt);
                alt_group_set_values(group, alt, vs);
                basic_hud_set(hud, 1, ALTITUDE, alt);
            }
            break;
        case SDLK_DOWN:
            if(event->state == SDL_PRESSED){
                vs = compute_vs(alt, alt-ALT_INC, elapsed);
                alt -= ALT_INC;
                animated_gauge_set_value(ANIMATED_GAUGE(ladder), alt);
                odo_gauge_set_value(wheel, alt);
                alt_indicator_set_value(alt_ind, alt);
                //alt_group_set_altitude(group, alt);
                alt_group_set_values(group, alt, vs);
                basic_hud_set(hud, 1, ALTITUDE, alt);
            }
            break;
        case SDLK_l:
            if(event->state == SDL_PRESSED){
                odo_gauge_set_value(wheel, 300);
            }
            break;
        case SDLK_PAGEUP:
            if(event->state == SDL_PRESSED){
                if(odo_val < odo->max_value)
                    odo_val += ODO_INC;
                odo_gauge_set_value(odo, odo_val);
                odo_gauge_set_value(gauge, odo_val);
            }
            break;
        case SDLK_PAGEDOWN:
            if(event->state == SDL_PRESSED){
                if(odo_val > 0.0)
                    odo_val -= ODO_INC;
                odo_gauge_set_value(odo, odo_val);
                odo_gauge_set_value(gauge, odo_val);
            }
            break;
        case SDLK_p:
            if(event->state == SDL_PRESSED){
//                if(vs < stair->scale.end)
                    vs += VARIO_INC;
                animated_gauge_set_value(ANIMATED_GAUGE(stair), vs);
                alt_group_set_vertical_speed(group, vs);
                basic_hud_set(hud, 1, VERTICAL_SPEED, vs);
            }
            break;
        case SDLK_m:
            if(event->state == SDL_PRESSED){
//                if(vs > stair->scale.start)
                    vs -= VARIO_INC;
                animated_gauge_set_value(ANIMATED_GAUGE(stair), vs);
                alt_group_set_vertical_speed(group, vs);
                basic_hud_set(hud, 1, VERTICAL_SPEED, vs);
            }
            break;
        case SDLK_x:
            if(event->state == SDL_PRESSED){
                    pitch -= PITCH_INC;
                animated_gauge_set_value(ANIMATED_GAUGE(ai), pitch);
                basic_hud_set(hud, 1, PITCH, pitch);
            }
            break;
        case SDLK_s:
            if(event->state == SDL_PRESSED){
                    pitch += PITCH_INC;
                animated_gauge_set_value(ANIMATED_GAUGE(ai), pitch);
                basic_hud_set(hud, 1, PITCH, pitch);
            }
            break;
        case SDLK_c:
            if(event->state == SDL_PRESSED){
                roll -= ROLL_INC;
                attitude_indicator_set_roll(ai, roll);
                animated_gauge_set_value(ANIMATED_GAUGE(rsg), roll);
                basic_hud_set(hud, 1, ROLL, roll);
            }
            break;
        case SDLK_d:
            if(event->state == SDL_PRESSED){
                roll += ROLL_INC;
                attitude_indicator_set_roll(ai, roll);
                animated_gauge_set_value(ANIMATED_GAUGE(rsg), roll);
                basic_hud_set(hud, 1, ROLL, roll);
            }
            break;

    }
    return false;
}

/*Return true to quit the app*/
bool handle_events(Uint32 elapsed)
{
    SDL_Event event;

    while(SDL_PollEvent(&event) == 1){
        switch(event.type){
            case SDL_QUIT:
                return true;
                break;
        case SDL_WINDOWEVENT:
            if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                return true;
            break;
            case SDL_KEYUP:
            case SDL_KEYDOWN:
                return handle_keyboard(&(event.key), elapsed);
                break;
        }
    }
    return false;
}

float compute_vs(float old_alt, float new_alt, Uint32 elapsed)
{
    return vs;
    float rv;

    rv = (new_alt-old_alt)/(elapsed/1000); /*ft per second*/
    rv *= 60; /*ft/min*/

    return rv;
}


int main(int argc, char **argv)
{
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;
    Uint32 colors[N_COLORS];
    bool done;
    int i;
    float oldv[5] = {0,0,0,0,0};

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }

    if( TTF_Init() == -1 ){
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }


    window = SDL_CreateWindow(
                "hello_sdl2",
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                SDL_WINDOW_SHOWN
                );
    if (window == NULL) {
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return 1;
    }

    screenSurface = SDL_GetWindowSurface(window);
    if(!screenSurface){
        printf("Error: %s\n",SDL_GetError());
        exit(-1);
    }


    colors[0] = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
    colors[1] = SDL_MapRGB(screenSurface->format, 0xFF, 0x00, 0x00);
    colors[2] = SDL_MapRGB(screenSurface->format, 0x00, 0xFF, 0x00);
    colors[3] = SDL_MapRGB(screenSurface->format, 0x11, 0x56, 0xFF);

    gauge = odo_gauge_new(digit_barrel_new(61, 0, 99,10),-1,-1);
    odo_gauge_set_value(gauge, gval);

    ladder = ladder_gauge_new((LadderPageDescriptor *)alt_ladder_page_descriptor_new(), -1);
    animated_gauge_set_value(ANIMATED_GAUGE(ladder), alt);

    DigitBarrel *db = digit_barrel_new(18, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(18, 0, 99, 10);
    wheel = odo_gauge_new_multiple(-1, 4,
            -1, db2,
            -2, db,
            -2, db,
            -2, db
    );
    odo_gauge_set_value(wheel, alt);

    DigitBarrel *db4 = digit_barrel_new(18, 0, 9.999, 1);
    DigitBarrel *db3 = digit_barrel_new(18, 0, 99, 10);
    odo = odo_gauge_new_multiple(-1, 3,
            -1, db3,
            -2, db4,
            -2, db4
    );
    odo_gauge_set_value(odo, odo_val);

    alt_ind = alt_indicator_new();
    alt_indicator_set_value(alt_ind, alt);

    stair = vertical_stair_new("vs-bg.png","vs-cursor.png", 16);
    animated_gauge_set_value(ANIMATED_GAUGE(stair), vs);

    group = alt_group_new();
    alt_group_set_values(group, alt, vs);


    asi = airspeed_indicator_new(50,60,85,155,200);
    airspeed_indicator_set_value(asi, ias);

    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;
    Uint32 acc = 0;
    i = 3;

    SDL_Rect dst = {SCREEN_WIDTH/2,SCREEN_HEIGHT/2,0,0};
    SDL_Rect lrect = {150,20,0,0};
    SDL_Rect wheelrect = {400,20,0,0};
    SDL_Rect odorect = {SCREEN_WIDTH/2 - 80,SCREEN_HEIGHT/2,0,0};
 //   SDL_Rect airect = {SCREEN_WIDTH/2 + 90,SCREEN_HEIGHT/2-20,0,0};
    SDL_Rect attrect = {0,0,0,0};

    ai = attitude_indicator_new(640,480);
	animated_gauge_set_value(ANIMATED_GAUGE(ai), pitch);
    attitude_indicator_set_roll(ai, roll);

    rsg = roll_slip_gauge_new();
    animated_gauge_set_value(ANIMATED_GAUGE(rsg), roll);

    hud = basic_hud_new();

    SDL_Rect airect = {439,50,0,0};
    SDL_Rect vrect = {96,70,0,0};

    SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;
        acc += elapsed;

        done = handle_events(elapsed);

        /* Not having this in the loop breaks ladder-gauge display
         * TODO: Check why and fix
         * */
        SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(ladder), elapsed) , NULL, screenSurface, &lrect);
//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(gauge), elapsed) , NULL, screenSurface, &dst);
//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(wheel), elapsed) , NULL, screenSurface, &wheelrect);
//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(odo), elapsed) , NULL, screenSurface, &odorect);

//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(alt_ind), elapsed) , NULL, screenSurface, &airect);
//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(stair), elapsed) , NULL, screenSurface, &vrect);

//        alt_group_render_at(group, elapsed, screenSurface, &airect);
        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(asi), elapsed), NULL, screenSurface, &vrect);
//

//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(rsg), elapsed) , NULL, screenSurface, &dst);
//        base_gauge_render(BASE_GAUGE(ai->rollslip), elapsed);
//        SDL_BlitSurface(base_gauge_render(BASE_GAUGE(ai), elapsed) , NULL, screenSurface, NULL);

//        basic_hud_render(hud, elapsed, screenSurface);
        SDL_UpdateWindowSurface(window);

        if(elapsed < 200){
            SDL_Delay(200 - elapsed);
        }
        if(acc >= 1000){
            if(ANIMATED_GAUGE(ladder)->value != oldv[0]){
                printf("Ladder value: %f\n",ANIMATED_GAUGE(ladder)->value);
                oldv[0] = ANIMATED_GAUGE(ladder)->value;
            }
            if(ANIMATED_GAUGE(gauge)->value != oldv[1]){
                printf("Rotary gauge value: %f\n",ANIMATED_GAUGE(gauge)->value);
                oldv[1] = ANIMATED_GAUGE(gauge)->value;
            }
            if(ANIMATED_GAUGE(odo)->value != oldv[2]){
                printf("Odo gauge value: %f\n",ANIMATED_GAUGE(odo)->value);
                oldv[2] = ANIMATED_GAUGE(odo)->value;
            }
            if(ANIMATED_GAUGE(ai)->value != oldv[3]){
                printf("Attitude pitch: %0.2f\n",ANIMATED_GAUGE(ai)->value);
                oldv[3] = ANIMATED_GAUGE(ai)->value;
            }
#if 1
            if(ANIMATED_GAUGE(rsg)->value != oldv[4]){
                printf("roll value: %0.2f\n",ANIMATED_GAUGE(rsg)->value);
                oldv[4] = ANIMATED_GAUGE(rsg)->value;
            }
#else
            if(ai->rollslip->parent.value != oldv[4]){
                printf("Attitude roll: %0.2f\n", ai->rollslip->parent.value);
                oldv[4] = ai->rollslip->parent.value;
            }
#endif

            acc = 0;
        }

        i %= N_COLORS;
        last_ticks = ticks;
    }while(!done);

    odo_gauge_free(gauge);
    odo_gauge_free(wheel);
    odo_gauge_free(odo);
    ladder_gauge_free(ladder);
    alt_group_free(group);
    airspeed_indicator_free(asi);
    alt_indicator_free(alt_ind);
    vertical_stair_free(stair);
    roll_slip_gauge_free(rsg);
    attitude_indicator_free(ai);
    basic_hud_free(hud);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;

}

