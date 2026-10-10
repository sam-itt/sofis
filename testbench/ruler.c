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

#include "generic-layer.h"
#include "generic-ruler.h"
#include "resource-manager.h"

#include "sdl-colors.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

//#define SCREEN_WIDTH 800
//#define SCREEN_HEIGHT 480

#define TEST_2 1




/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{

    if(event->state != SDL_PRESSED) return false;
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            return true;
            break;
        case SDLK_UP:
            break;
        case SDLK_DOWN:
            break;
        case SDLK_SPACE:
            break;
        default:{
        }
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
    GenericRuler *ruler;

    ruler = calloc(1, sizeof(GenericRuler));
#if OLD_IMPL
    generic_ruler_init(ruler, RulerVertical, RulerGrowAgainstAxis, 300, 2700, 300, resource_manager_get_font(TERMINUS_12), Left, 0, 20, 120);
    Uint32 white = SDL_UWHITE(GENERIC_LAYER(ruler)->canvas);
    generic_ruler_etch_hatches(ruler, white, false, true, Left);
    generic_ruler_etch_markings(ruler, Left, resource_manager_get_font(TERMINUS_12), white, 0);

    SDL_Surface *canvas = GENERIC_LAYER(ruler)->canvas;
    SDL_SaveBMP(canvas, "/tmp/ruler.bmp");
    printf("Ruler area is:\n");
    SDLExt_RectDump(&ruler->ruler_area);
    exit(0);
#else
#if TEST_1
    generic_ruler_init(ruler, 59/*29*/, 191 /*165*/, RulerVertical, RulerGrowAgainstAxis,
                        /*500*/ 2.5, 21, 5, true,
                        1, (RulerUnitMultiple[]){{.value=2,.size=10, .labeled=true}},
//                        (Anchor){.value=1500,.position=32},
                        (Anchor){.value=0,.position=95});
#elif TEST_2
    generic_ruler_init(ruler, 59/*29*/, 191 /*165*/, RulerVertical, RulerGrowAgainstAxis,
                        500, 21, 5, false,
                        1, (RulerUnitMultiple[]){{.value=2, .size=10, .labeled=true}},
//                        (Anchor){.value=1500,.position=32},
                        (Anchor){.value=0,.position=95});

    generic_ruler_etch_hatches(ruler, SDL_UWHITE(GENERIC_LAYER(ruler)->canvas), true, Left);
    generic_ruler_etch_markings(ruler, Right, resource_manager_get_font(TERMINUS_12), SDL_UWHITE(GENERIC_LAYER(ruler)->canvas), 1);
    generic_layer_save_bmp(GENERIC_LAYER(ruler), "/tmp/ruler.bmp");
    exit(0);
#elif TEST_3

//        150, 13, /* 76x9 on the screenshot*/
    generic_ruler_init(ruler, 150, 40, RulerHorizontal, RulerGrowAgainstAxis, 5, 29,
                        1, (RulerUnitMultiple[]){{.value=2,.size=10, .labeled=true}},
//                        (Anchor){.value=1500,.position=32},
                        (Anchor){.value=0,.position=149});

    generic_ruler_etch_hatches(ruler, SDL_UWHITE(GENERIC_LAYER(ruler)->canvas), true, Center);
    generic_ruler_etch_markings(ruler, Top|Bottom, resource_manager_get_font(TERMINUS_12), SDL_UWHITE(GENERIC_LAYER(ruler)->canvas));
    exit(0);
#elif TEST_4
#else
    printf("No test defined, exiting\n");
    exit(0);
#endif
#endif //OLD_IMPL
    SDL_Rect center_rect = {(640-1)/2,(480-1)/2,0,0};

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

    resource_manager_shutdown();
#if USE_SDL_GPU
	GPU_Quit();
#else
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;

}

