#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "animated-gauge.h"
#include "basic-hud.h"
#include "flightgear-connector.h"
#include "ladder-gauge.h"
#include "alt-ladder-page-descriptor.h"

#include "odo-gauge.h"
#include "alt-indicator.h"
#include "vertical-stair.h"
#include "alt-group.h"
#include "airspeed-indicator.h"
#include "attitude-indicator.h"
#include "resource-manager.h"

#include "roll-slip-gauge.h"


#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define N_COLORS 4

BasicHud *hud = NULL;


float alt = 900.0;
float odo_val = 0.0;
float vs = 0.0;
float ias = 10.0;
float pitch = 0.0;
float roll = 0.0;
#define VARIO_INC 100
#define IAS_INC 1
#define ALT_INC 150
#define PITCH_INC 1;
#define ROLL_INC 1;


/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{
    switch(event->keysym.sym){
        case SDLK_ESCAPE:
            if(event->state == SDL_PRESSED)
                return true;
            break;
        case SDLK_a:
            if(event->state == SDL_PRESSED){
                ias += IAS_INC;
                basic_hud_set(hud, 1, AIRSPEED, ias);
            }
            break;
        case SDLK_z:
            if(event->state == SDL_PRESSED){
                ias -= IAS_INC;
                basic_hud_set(hud, 1, AIRSPEED, ias);
            }
            break;
        case SDLK_UP:
            if(event->state == SDL_PRESSED){
                alt += ALT_INC;
                basic_hud_set(hud, 1, ALTITUDE, alt);
            }
            break;
        case SDLK_DOWN:
            if(event->state == SDL_PRESSED){
                alt -= ALT_INC;
                basic_hud_set(hud, 1, ALTITUDE, alt);
            }
            break;
        case SDLK_PAGEUP:
            if(event->state == SDL_PRESSED){
                vs += VARIO_INC;
                basic_hud_set(hud, 1, VERTICAL_SPEED, vs);
            }
            break;
        case SDLK_PAGEDOWN:
            if(event->state == SDL_PRESSED){
                vs -= VARIO_INC;
                basic_hud_set(hud, 1, VERTICAL_SPEED, vs);
            }
            break;
        case SDLK_x:
            if(event->state == SDL_PRESSED){
                pitch -= PITCH_INC;
                basic_hud_set(hud, 1, PITCH, pitch);
            }
            break;
        case SDLK_s:
            if(event->state == SDL_PRESSED){
                pitch += PITCH_INC;
                basic_hud_set(hud, 1, PITCH, pitch);
            }
            break;
        case SDLK_c:
            if(event->state == SDL_PRESSED){
                roll -= ROLL_INC;
                basic_hud_set(hud, 1, ROLL, roll);
            }
            break;
        case SDLK_d:
            if(event->state == SDL_PRESSED){
                roll += ROLL_INC;
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
                "HUD testbench",
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

    SDL_Rect whole = {0,0,640,480};
    hud = basic_hud_new();

    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;
    Uint32 acc = 0;
    i = 3;

    FlightgearConnector *fglink;
    fglink = flightgear_connector_new(6789);
    flightgear_connector_set_nonblocking(fglink);
    FlightgearPacket packet;

    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;
        acc += elapsed;

        done = handle_events(elapsed);
        if(flightgear_connector_get_packet(fglink, &packet)){
            basic_hud_set(hud,  5,
                ALTITUDE, (float)packet.altitude,
                AIRSPEED, (float)packet.airspeed,
                VERTICAL_SPEED, packet.vertical_speed,
                PITCH, packet.pitch,
                ROLL, packet.roll
            );
        }

        SDL_FillRect(screenSurface, NULL, colors[i]);

        base_gauge_render(BASE_GAUGE(hud), elapsed, screenSurface, &whole);
        SDL_UpdateWindowSurface(window);

        if(elapsed < 200){
            SDL_Delay(200 - elapsed);
        }
        if(acc >= 1000){
            float tmp;

            tmp = basic_hud_get(hud, ALTITUDE);
            if(tmp != oldv[0]){
                printf("Altitude: %f\n", tmp);
                oldv[0] = tmp;
            }

            tmp = basic_hud_get(hud, VERTICAL_SPEED);
            if(tmp != oldv[1]){
                printf("Vertical speed: %f\n", tmp);
                oldv[1] = tmp;
            }

            tmp = basic_hud_get(hud, AIRSPEED);
            if(tmp != oldv[2]){
                printf("Airspeed: %f\n", tmp);
                oldv[2] = tmp;
            }

            tmp = basic_hud_get(hud, PITCH);
            if(tmp != oldv[3]){
                printf("Pitch: %f\n", tmp);
                oldv[3] = tmp;
            }

            tmp = basic_hud_get(hud, ROLL);
            if(tmp != oldv[4]){
                printf("Roll: %f\n", tmp);
                oldv[4] = tmp;
            }

            acc = 0;
        }

        i %= N_COLORS;
        last_ticks = ticks;
    }while(!done);

    basic_hud_free(hud);
    flightgear_connector_free(fglink);

    resource_manager_shutdown();
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;

}

