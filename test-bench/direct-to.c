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

#include <SDL2/SDL.h>
#include <SDL_gpu.h>

#include "base-gauge.h"
#include "base-widget.h"
#include "direct-to-dialog.h"
#include "mock-data-source.h"


#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480


#define WINDOW_TITLE "Direct To Dialog Test"

#define N_COLORS 4


DirectToDialog *direct = NULL;




/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{

    if(event->state != SDL_PRESSED) return false;
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            return true;
            break;

    }
    if(direct->visible)
        base_widget_handle_event(BASE_WIDGET(direct), event);


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
                WINDOW_TITLE,
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


#if USE_SDL_GPU
    GPU_ClearRGB(gpu_screen, 0x11, 0x56, 0xFF);
#else
    SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
#endif

    direct = direct_to_dialog_new();
    SDL_Rect direct_rect ={
        640/2,
        480/2 - 100,
        base_gauge_w(BASE_GAUGE(direct)),
        base_gauge_h(BASE_GAUGE(direct))
    };
    direct->visible = true;


    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;
    Uint32 acc = 0;
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
        if(direct->visible)
            base_gauge_render(BASE_GAUGE(direct), elapsed, &(RenderContext){rtarget, &direct_rect, NULL});
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
        last_ticks = ticks;
    }while(!done);

    base_gauge_free(BASE_GAUGE(direct));
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

