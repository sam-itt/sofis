#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

//#include "animated-gauge.h"
#include "base-gauge.h"
#include "basic-hud.h"
#include "compass-gauge.h"
#include "fishbone-gauge.h"
#include "ladder-gauge.h"
#include "alt-ladder-page-descriptor.h"

#include "odo-gauge.h"
#include "alt-indicator.h"
#include "resource-manager.h"
#include "SDL_pcf.h"
#include "side-panel.h"
#include "text-gauge.h"
#include "vertical-stair.h"
#include "alt-group.h"
#include "airspeed-indicator.h"
#include "attitude-indicator.h"

#include "roll-slip-gauge.h"

#include "sdl-colors.h"
#include "rich-compass-gauge.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

//#define SCREEN_WIDTH 800
//#define SCREEN_HEIGHT 480

#define N_COLORS 4

/*BasicHud *hud = NULL;*/
OdoGauge *gauge = NULL;
LadderGauge *ladder = NULL;
OdoGauge *wheel = NULL;
OdoGauge *odo = NULL;
/*AltIndicator *alt_ind = NULL;*/
/*VerticalStair *stair = NULL;*/
AltGroup *group = NULL;
AirspeedIndicator *asi = NULL;
/*AttitudeIndicator *ai = NULL;*/
RollSlipGauge *rsg = NULL;
TextGauge *txt = NULL;
/*ElevatorGauge *elevator = NULL;*/
/*RichCompassGauge *compass = NULL;*/


float gval = 0.0;
float alt = 900.0;
float odo_val = 0.0;
//float vs = 2000.0;
float vs = 0.0;
float ias = 10.0;
//float pitch = 2.19;
float pitch = 0.0;
float roll = 0.0;
char txtbuf[256];
float heading = 0.0;

#define ODO_INC 1
#define VARIO_INC 100
#define IAS_INC 1
#define ALT_INC 150
#define PITCH_INC 1;
#define ROLL_INC 1;
#define GVAL_INC 10.0;
#define HEADING_INC 5.0;

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
                odo_gauge_set_value(gauge, gval, true);
//                printf("Odo gauge just set to %0.2f\n",gval);
            }
            break;
        case SDLK_g:
            if(event->state == SDL_PRESSED){
                gval -= GVAL_INC;
                if(gval < 0)
                    gval = 0;
                odo_gauge_set_value(gauge, gval, true);
//                printf("Odo gauge just set to %0.2f\n",gval);
            }
            break;
        case SDLK_a:
            if(event->state == SDL_PRESSED){
                ias += IAS_INC;
                airspeed_indicator_set_value(asi, ias);
                /*basic_hud_set(hud, 1, AIRSPEED, ias);*/
            }
            break;
        case SDLK_z:
            if(event->state == SDL_PRESSED){
                ias -= IAS_INC;
                airspeed_indicator_set_value(asi, ias);
                /*basic_hud_set(hud, 1, AIRSPEED, ias);*/
            }
            break;
        case SDLK_UP:
            if(event->state == SDL_PRESSED){
                /*vs = compute_vs(alt, alt+ALT_INC, elapsed);*/
                alt += ALT_INC;
                ladder_gauge_set_value(ladder, alt, true);
                odo_gauge_set_value(wheel, alt, true);
                /*alt_indicator_set_value(alt_ind, alt, true);*/
                //alt_group_set_altitude(group, alt);
                alt_group_set_values(group, alt, vs);
                /*basic_hud_set(hud, 1, ALTITUDE, alt);*/
                sprintf(txtbuf, "Altitude: %0.2f", alt);
                text_gauge_set_value(txt, txtbuf);
            }
            break;
        case SDLK_DOWN:
            if(event->state == SDL_PRESSED){
                /*vs = compute_vs(alt, alt-ALT_INC, elapsed);*/
                alt -= ALT_INC;
                ladder_gauge_set_value(ladder, alt, true);
                odo_gauge_set_value(wheel, alt, true);
                /*alt_indicator_set_value(alt_ind, alt, true);*/
                //alt_group_set_altitude(group, alt);
                alt_group_set_values(group, alt, vs);
                /*basic_hud_set(hud, 1, ALTITUDE, alt);*/
                sprintf(txtbuf, "Altitude: %0.2f", alt);
                text_gauge_set_value(txt, txtbuf);
            }
            break;
        case SDLK_l:
            if(event->state == SDL_PRESSED){
                odo_gauge_set_value(wheel, 300, true);
            }
            break;
        case SDLK_PAGEUP:
            if(event->state == SDL_PRESSED){
                if(odo_val < odo->max_value)
                    odo_val += ODO_INC;
                odo_gauge_set_value(odo, odo_val, true);
                odo_gauge_set_value(gauge, odo_val, true);
            }
            break;
        case SDLK_PAGEDOWN:
            if(event->state == SDL_PRESSED){
                if(odo_val > 0.0)
                    odo_val -= ODO_INC;
                odo_gauge_set_value(odo, odo_val, true);
                odo_gauge_set_value(gauge, odo_val, true);
            }
            break;
        case SDLK_p:
            if(event->state == SDL_PRESSED){
//                if(vs < stair->scale.end)
                    vs += VARIO_INC;
                /*vertical_stair_set_value(stair, vs, true);*/
                alt_group_set_vertical_speed(group, vs);
                /*basic_hud_set(hud, 1, VERTICAL_SPEED, vs);*/
            }
            break;
        case SDLK_m:
            if(event->state == SDL_PRESSED){
//                if(vs > stair->scale.start)
                    vs -= VARIO_INC;
                /*vertical_stair_set_value(stair, vs, true);*/
                alt_group_set_vertical_speed(group, vs);
                /*basic_hud_set(hud, 1, VERTICAL_SPEED, vs);*/
            }
            break;
        /*case SDLK_x:*/
            /*if(event->state == SDL_PRESSED){*/
                    /*pitch -= PITCH_INC;*/
                /*animated_gauge_set_value(ANIMATED_GAUGE(ai), pitch);*/
                /*basic_hud_set(hud, 1, PITCH, pitch);*/
            /*}*/
            /*break;*/
        /*case SDLK_s:*/
            /*if(event->state == SDL_PRESSED){*/
                    /*pitch += PITCH_INC;*/
                /*animated_gauge_set_value(ANIMATED_GAUGE(ai), pitch);*/
                /*basic_hud_set(hud, 1, PITCH, pitch);*/
            /*}*/
            /*break;*/
        case SDLK_c:
            if(event->state == SDL_PRESSED){
                roll -= ROLL_INC;
                /*attitude_indicator_set_roll(ai, roll);*/
                roll_slip_gauge_set_value(rsg, roll, true);
                /*basic_hud_set(hud, 1, ROLL, roll);*/
            }
            break;
        case SDLK_d:
            if(event->state == SDL_PRESSED){
                roll += ROLL_INC;
                /*attitude_indicator_set_roll(ai, roll);*/
                roll_slip_gauge_set_value(rsg, roll, true);
                /*basic_hud_set(hud, 1, ROLL, roll);*/
            }
            break;
        /*case SDLK_e:*/
            /*if(event->state == SDL_PRESSED){*/
                /*heading += HEADING_INC;*/
                /*heading = fmod(heading, 360.0);*/
                /*if(heading < 0)*/
                    /*heading += 360.0;*/
                /*animated_gauge_set_value(ANIMATED_GAUGE(compass->compass), heading);*/
            /*}*/
            /*break;*/
        case SDLK_SPACE:
            if(event->state == SDL_PRESSED){
                printf("Alt is: %f\n", alt);
                printf("Roll is: %f\n", roll);
//                printf("TextGauge value: %s\n", txt->value);
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
    Uint32 colors[N_COLORS];
    bool done;
    int i;
    float oldv[5] = {0,0,0,0,0};
    RenderTarget rtarget;

#if USE_SDL_GPU
    GPU_Target* gpu_screen = NULL;

	GPU_SetRequiredFeatures(GPU_FEATURE_BASIC_SHADERS);
	gpu_screen = GPU_InitRenderer(GPU_RENDERER_OPENGL_2, SCREEN_WIDTH, SCREEN_HEIGHT, GPU_DEFAULT_INIT_FLAGS);
	if(gpu_screen == NULL){
        GPU_LogError("Initialization Error: Could not create a renderer with proper feature support for this demo.\n");
		return 1;
    }
    rtarget.target = gpu_screen;
#else
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }
//    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
//    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, "0", SDL_HINT_OVERRIDE);

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
    rtarget.surface = screenSurface;
    colors[0] = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
    colors[1] = SDL_MapRGB(screenSurface->format, 0xFF, 0x00, 0x00);
    colors[2] = SDL_MapRGB(screenSurface->format, 0x00, 0xFF, 0x00);
    colors[3] = SDL_MapRGB(screenSurface->format, 0x11, 0x56, 0xFF);
#endif

    gauge = odo_gauge_new(digit_barrel_new(
        resource_manager_get_font(TERMINUS_32), 0, 99,10),
        -1,-1
    );
    odo_gauge_set_value(gauge, gval, true);

    ladder = ladder_gauge_new((LadderPageDescriptor *)alt_ladder_page_descriptor_new(), -1);
    ladder_gauge_set_value(ladder, alt, true);

    PCF_Font *fnt = resource_manager_get_font(TERMINUS_18);
    DigitBarrel *db = digit_barrel_new(fnt, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(fnt, 0, 99, 10);
    wheel = odo_gauge_new_multiple(-1, 4,
            -1, db2,
            -2, db,
            -2, db,
            -2, db
    );
    odo_gauge_set_value(wheel, alt, true);

    DigitBarrel *db4 = digit_barrel_new(fnt, 0, 9.999, 1);
    DigitBarrel *db3 = digit_barrel_new(fnt, 0, 99, 10);
    odo = odo_gauge_new_multiple(-1, 3,
            -1, db3,
            -2, db4,
            -2, db4
    );
    odo_gauge_set_value(odo, odo_val, true);

    /*alt_ind = alt_indicator_new();*/
    /*alt_indicator_set_value(alt_ind, alt, true);*/

/*    stair = vertical_stair_new(*/
        /*"vs-bg.png",*/
        /*"vs-cursor.png",*/
        /*resource_manager_get_static_font(TERMINUS_16, &SDL_WHITE, 2, PCF_DIGITS, "+-")*/
    /*);*/
    /*vertical_stair_set_value(stair, vs, true);*/

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
//    SDL_Rect lrect = {0,0,0,0};
    SDL_Rect wheelrect = {400,20,0,0};
    SDL_Rect odorect = {SCREEN_WIDTH/2 - 80,SCREEN_HEIGHT/2,0,0};
 //   SDL_Rect airect = {SCREEN_WIDTH/2 + 90,SCREEN_HEIGHT/2-20,0,0};
    SDL_Rect attrect = {0,0,0,0};

/*    ai = attitude_indicator_new(640,480);*/
/*//    animated_gauge_set_value(ANIMATED_GAUGE(ai), pitch);*/
    /*attitude_indicator_set_roll(ai, roll);*/

    rsg = roll_slip_gauge_new();
    roll_slip_gauge_set_value(rsg, roll, true);

    /*hud = basic_hud_new();*/

    txt = text_gauge_new("HI THERE", true, 300, 30);
#if 1
    text_gauge_set_static_font(txt,
        resource_manager_get_static_font(TERMINUS_24,
            &SDL_WHITE,
            3, PCF_ALPHA, PCF_DIGITS, ".:"
        )
    );
#else
    text_gauge_set_font(txt,
        resource_manager_get_font(TERMINUS_24)
    );
#endif
    text_gauge_set_color(txt, SDL_WHITE, TEXT_COLOR);
    text_gauge_set_color(txt, SDL_BLACK, BACKGROUND_COLOR);
    SDL_Rect txtrect = {SCREEN_WIDTH/2.0, SCREEN_HEIGHT/2.0,0,0};

    /*//compass = compass_gauge_new();*/
    /*compass = rich_compass_gauge_new();*/
#if 0
    elevator = elevator_gauge_new(
        true, Left,
        resource_manager_get_font(TERMINUS_12),
        SDL_WHITE,
        300, 2700, 300,
        20, 120, /* 10x60 on the screenshot*/
        3,(ColorZone[]){{
            .from = 300,
            .to = 900,
            .color = SDL_GREEN,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 900,
        .to = 2000,
        .color = SDL_YELLOW,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 2000,
        .to = 2300,
        .color = SDL_RED,
        .flags = FromExcluded | ToIncluded
        }}
    );
#endif
#if 0
    FishboneGauge *fish = fishbone_gauge_new(
        true,
        resource_manager_get_font(TERMINUS_12),
        SDL_WHITE,
        0, 25, 5,
        150, 13, /* 76x9 on the screenshot*/
        3,(ColorZone[]){{
            .from = 0,
            .to = 2,
            .color = SDL_RED,
            .flags = FromIncluded | ToIncluded
        },{
        .from = 2,
        .to = 10,
        .color = SDL_YELLOW,
        .flags = FromExcluded | ToIncluded
        },{
        .from = 10,
        .to = 25,
        .color = SDL_GREEN,
        .flags = FromExcluded | ToIncluded
        }}
    );
#endif

    SDL_Rect airect = {439,50,0,0};
    SDL_Rect vrect = {96,70,0,0};
    SDL_Rect whole = {0,0,SCREEN_WIDTH,SCREEN_HEIGHT};
    SDL_Rect center_rect = {(640-1)/2,(480-1)/2,0,0};
//    SDL_Rect center_rect = {0,0,0,0};
#if 1
/*    SidePanel *panel;*/
    /*panel = side_panel_new(-1, -1);*/
    /*SDL_Rect sprect = {50,0, BASE_GAUGE(panel)->w,BASE_GAUGE(panel)->h};*/
#endif
#if USE_SDL_GPU
    GPU_ClearRGB(gpu_screen, 0x11, 0x56, 0xFF);
#else
    SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
#endif

    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;
        acc += elapsed;

        done = handle_events(elapsed);

        /* Not having this in the loop breaks ladder-gauge display
         * TODO: Check why and fix
         * */
#if USE_SDL_GPU
        GPU_ClearRGB(gpu_screen, 0x11, 0x56, 0xFF);
#else
        SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
#endif
//        base_gauge_render(BASE_GAUGE(ladder), elapsed, &(RenderContext){rtarget, &lrect, NULL});
//void base_gauge_render(BaseGauge *self, Uint32 dt, RenderTarget destination, SDL_Rect *location, SDL_Rect *portion);
//        base_gauge_render(BASE_GAUGE(gauge), elapsed, &(RenderContext){rtarget, &dst, NULL});
//        base_gauge_render(BASE_GAUGE(wheel), elapsed, &(RenderContext){rtarget, &wheelrect, NULL});
//        base_gauge_render(BASE_GAUGE(odo), elapsed, &(RenderContext){rtarget, &odorect, NULL});

        /*base_gauge_render(BASE_GAUGE(alt_ind), elapsed, &(RenderContext){rtarget, &airect, NULL});*/
        /*base_gauge_render(BASE_GAUGE(stair), elapsed, &(RenderContext){rtarget, &vrect, NULL});*/

        base_gauge_render(BASE_GAUGE(group), elapsed, &(RenderContext){rtarget, &airect, NULL});
        base_gauge_render(BASE_GAUGE(asi), elapsed, &(RenderContext){rtarget, &vrect, NULL});
//

        base_gauge_render(BASE_GAUGE(rsg), elapsed, &(RenderContext){rtarget, &dst, NULL});
//        base_gauge_render(BASE_GAUGE(ai->rollslip), elapsed);
//        base_gauge_render(BASE_GAUGE(ai), elapsed, rtarget, NULL);

//        base_gauge_render(BASE_GAUGE(hud), elapsed, rtarget, &whole);
//        base_gauge_render(BASE_GAUGE(txt), elapsed, &(RenderContext){rtarget, &txtrect, NULL});
//        base_gauge_render(BASE_GAUGE(panel), elapsed, rtarget, &sprect);
//        base_gauge_render(BASE_GAUGE(elevator), elapsed, rtarget, &center_rect);
//        base_gauge_render(BASE_GAUGE(fish), elapsed, rtarget, &center_rect);
//        base_gauge_render(BASE_GAUGE(compass), elapsed, rtarget, &center_rect);
#if USE_SDL_GPU
		GPU_Flip(gpu_screen);
#else
        SDL_UpdateWindowSurface(window);
#endif
#if 1
        if(elapsed < 200){
            SDL_Delay(200 - elapsed);
        }
#endif
        if(acc >= 10000){
            if(ladder->value != oldv[0]){
                printf("Ladder value: %f\n",ladder->value);
                oldv[0] = ladder->value;
            }
            /*if(ANIMATED_GAUGE(gauge)->value != oldv[1]){*/
                /*printf("Rotary gauge value: %f\n",ANIMATED_GAUGE(gauge)->value);*/
                /*oldv[1] = ANIMATED_GAUGE(gauge)->value;*/
            /*}*/
            /*if(ANIMATED_GAUGE(odo)->value != oldv[2]){*/
                /*printf("Odo gauge value: %f\n",ANIMATED_GAUGE(odo)->value);*/
                /*oldv[2] = ANIMATED_GAUGE(odo)->value;*/
            /*}*/
            /*if(ANIMATED_GAUGE(ai)->value != oldv[3]){*/
                /*printf("Attitude pitch: %0.2f\n",ANIMATED_GAUGE(ai)->value);*/
                /*oldv[3] = ANIMATED_GAUGE(ai)->value;*/
            /*}*/
#if 1
/*            if(ANIMATED_GAUGE(rsg)->value != oldv[4]){*/
                /*printf("roll value: %0.2f\n",ANIMATED_GAUGE(rsg)->value);*/
                /*oldv[4] = ANIMATED_GAUGE(rsg)->value;*/
            /*}*/
#else
            if(ai->rollslip->super.value != oldv[4]){
                printf("Attitude roll: %0.2f\n", ai->rollslip->super.value);
                oldv[4] = ai->rollslip->super.value;
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
    /*alt_indicator_free(alt_ind);*/
    /*vertical_stair_free(stair);*/
    roll_slip_gauge_free(rsg);
    /*attitude_indicator_free(ai);*/
    /*basic_hud_free(hud);*/
    text_gauge_free(txt);
    /*side_panel_free(panel);*/
    /*rich_compass_gauge_free(compass);*/
    resource_manager_shutdown();
#if USE_SDL_GPU
	GPU_Quit();
#else
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;

}

