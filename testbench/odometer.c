/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "base-gauge.h"
#include "odo-gauge.h"
#include "data-source.h"

#include "mock-data-source.h"
#include "resource-manager.h"
#include "SDL_pcf.h"



#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

//#define SCREEN_WIDTH 800
//#define SCREEN_HEIGHT 480


OdoGauge *gauge = NULL;
OdoGauge *wheel = NULL;
OdoGauge *odo = NULL;

float gval = 0.0;
float alt = 900.0;
float odo_val = 0.0;

#define GVAL_INC 10.0;
#define ODO_INC 1
#define ALT_INC 150



/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{


    if(event->state != SDL_PRESSED) return false;
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            return true;
            break;
        case SDLK_t:
            gval += GVAL_INC;
            if(gval > 99)
                gval = 99;
            odo_gauge_set_value(gauge, gval, true);
                printf("Odo gauge just set to %0.2f\n",gval);
            break;
        case SDLK_g:
            gval -= GVAL_INC;
            if(gval < 0)
                gval = 0;
            odo_gauge_set_value(gauge, gval, true);
                printf("Odo gauge just set to %0.2f\n",gval);
            break;
        case SDLK_UP:
            alt += ALT_INC;
            odo_gauge_set_value(wheel, alt, true);
            break;
        case SDLK_DOWN:
            alt -= ALT_INC;
            odo_gauge_set_value(wheel, alt, true);
            break;
        case SDLK_l:
            odo_gauge_set_value(wheel, 300, true);
            break;
        case SDLK_PAGEUP:
            if(odo_val < odo->max_value)
                odo_val += ODO_INC;
            odo_gauge_set_value(odo, odo_val, true);
            odo_gauge_set_value(gauge, odo_val, true);
            break;
        case SDLK_PAGEDOWN:
            if(odo_val > 0.0)
                odo_val -= ODO_INC;
            odo_gauge_set_value(odo, odo_val, true);
            odo_gauge_set_value(gauge, odo_val, true);
            break;
        case SDLK_SPACE:
            printf("Alt is: %f\n", alt);
            break;
        default:
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


int main(int argc, char **argv)
{
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
#endif
    data_source_set((DataSource*)mock_data_source_new());
    gauge = odo_gauge_new(digit_barrel_new(
        resource_manager_get_font(TERMINUS_32), 0, 99,10),
        -1,-1
    );
    odo_gauge_set_value(gauge, gval, true);


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




    SDL_Rect dst = {SCREEN_WIDTH/2,SCREEN_HEIGHT/2,0,0};
    SDL_Rect lrect = {150,20,0,0};
//    SDL_Rect lrect = {0,0,0,0};
    SDL_Rect wheelrect = {400,20,0,0};
    SDL_Rect odorect = {SCREEN_WIDTH/2 - 80,SCREEN_HEIGHT/2,0,0};


#if USE_SDL_GPU
    GPU_ClearRGB(gpu_screen, 0x11, 0x56, 0xFF);
#else
    SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
#endif




    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;
    Uint32 acc = 0;
    i = 3;
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

        base_gauge_render(BASE_GAUGE(gauge), elapsed, &(RenderContext){rtarget, &dst, NULL});
        base_gauge_render(BASE_GAUGE(wheel), elapsed, &(RenderContext){rtarget, &wheelrect, NULL});
        base_gauge_render(BASE_GAUGE(odo), elapsed, &(RenderContext){rtarget, &odorect, NULL});

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
            /*if(ANIMATED_GAUGE(gauge)->value != oldv[1]){*/
                /*printf("Rotary gauge value: %f\n",ANIMATED_GAUGE(gauge)->value);*/
                /*oldv[1] = ANIMATED_GAUGE(gauge)->value;*/
            /*}*/
            /*if(ANIMATED_GAUGE(odo)->value != oldv[2]){*/
                /*printf("Odo gauge value: %f\n",ANIMATED_GAUGE(odo)->value);*/
                /*oldv[2] = ANIMATED_GAUGE(odo)->value;*/
            /*}*/
            acc = 0;
        }

        last_ticks = ticks;
    }while(!done);

    base_gauge_free(BASE_GAUGE(gauge));
    base_gauge_free(BASE_GAUGE(wheel));
    base_gauge_free(BASE_GAUGE(odo));
    resource_manager_shutdown();
    data_source_free(data_source_get_instance());
#if USE_SDL_GPU
	GPU_Quit();
#else
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;

}

