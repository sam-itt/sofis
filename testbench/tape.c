/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "base-gauge.h"
#include "base-widget.h"
#include "basic-hud.h"
#include "button.h"
#include "softkey.h"
#include "compass-gauge.h"
#include "data-source.h"
#include "elevator-gauge.h"
#include "fishbone-gauge.h"
#include "ladder-gauge.h"
#include "alt-ladder-page-descriptor.h"

#include "ladder-page.h"
#include "mock-data-source.h"
#include "odo-gauge.h"
#include "alt-indicator.h"
#include "pfd-toplevel-softkey-model.h"
#include "resource-manager.h"
#include "SDL_pcf.h"
#include "side-panel.h"
#include "text-gauge.h"
#include "vertical-stair.h"
#include "alt-group.h"
#include "airspeed-indicator.h"
#include "attitude-indicator.h"
#include "roll-slip-gauge.h"
#include "tape-gauge.h"
#include "map-gauge.h"
#include "route-map-provider.h"
#include "direct-to-dialog.h"
#include "softkey-bar.h"


#include "sdl-colors.h"
#include "res-dirs.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

//#define SCREEN_WIDTH 800
//#define SCREEN_HEIGHT 480


TapeGauge *tape_gauge = NULL;
TapeGauge *tape_gauge2 = NULL;


float alt = 900.0;

#define ALT_INC 150



//typedef void (*EventListenerFunc)(void *self, BaseWidget *sender);
void button_soft_key_listener(EventListener *self, Softkey *sender)
{
    if(sender->state == SOFTKEY_STATE_RELEASED)
        softkey_set_state(sender, SOFTKEY_STATE_PRESSED);
    else
        softkey_set_state(sender, SOFTKEY_STATE_RELEASED);
}

/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{

    if(event->state != SDL_PRESSED) return false;
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            return true;
            break;
        case SDLK_UP:
            alt += ALT_INC;
            tape_gauge_set_value(tape_gauge, alt, true);
            break;
        case SDLK_DOWN:
            alt -= ALT_INC;
            tape_gauge_set_value(tape_gauge, alt, true);
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















    PCF_Font *fnt = resource_manager_get_font(TERMINUS_18);
    DigitBarrel *db = digit_barrel_new(fnt, 0, 9.999, 1);
    DigitBarrel *db2 = digit_barrel_new(fnt, 0, 99, 10);
    tape_gauge = tape_gauge_new(
        68, 240,
        (LadderPageDescriptor*)alt_ladder_page_descriptor_new(),
        AlignRight, 0, 4,
        -1, db2,
        -2, db,
        -2, db,
        -2, db
    );

    /*fnt = resource_manager_get_font(TERMINUS_18);*/
    /*db = digit_barrel_new(fnt, 0, 9.999, 1);*/
    /*tape_gauge2 = tape_gauge_new(*/
        /*68, 240,*/
        /*(LadderPageDescriptor*)airspeed_page_descriptor_new(50,60,85,155,200),*/
        /*AlignRight, -12, 3,*/
        /*-1, db,*/
        /*-2, db,*/
        /*-2, db*/
    /*);*/


    SDL_Rect vrect = {96,68,0,0};

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

        base_gauge_render(BASE_GAUGE(tape_gauge), elapsed, &(RenderContext){rtarget, &vrect, NULL});
        /*base_gauge_render(BASE_GAUGE(tape_gauge2), elapsed, &(RenderContext){rtarget, &vrect, NULL});*/

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
            acc = 0;
        }
        last_ticks = ticks;
    }while(!done);

    base_gauge_free(BASE_GAUGE(tape_gauge));
    /*base_gauge_free(BASE_GAUGE(tape_gauge2));*/
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

