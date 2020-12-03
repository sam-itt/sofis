#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

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
#include "resource-manager.h"

#include "roll-slip-gauge.h"
#include "side-panel.h"

#include "terrain-viewer.h"
//#define USE_FGCONN 0
#define USE_FGTAPE 1

#if defined(USE_FGCONN)
#include "flightgear-connector.h"
#elif defined(USE_FGTAPE)
#include "fg-tape.h"

typedef struct{
    double latitude;
    double longitude;
    double altitude;
    float roll;
    float pitch;
    float heading;
    float airspeed; //kts
    float vertical_speed; //vertical speed //feets per second

    float rpm;
    float fuel_flow;
    float oil_temp;
    float oil_press;
    float cht;
    float fuel_px;
    float fuel_qty;
}TapeRecord;
#endif

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define N_COLORS 4

BasicHud *hud = NULL;
SidePanel *panel = NULL;
bool g_show3d = false;


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
        case SDLK_SPACE:
            if(event->state == SDL_PRESSED)
                g_show3d = !g_show3d;
            hud->attitude->hide_ball = (g_show3d) ? true : false;
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
    rtarget.surface = screenSurface;

    colors[0] = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
    colors[1] = SDL_MapRGB(screenSurface->format, 0xFF, 0x00, 0x00);
    colors[2] = SDL_MapRGB(screenSurface->format, 0x00, 0xFF, 0x00);
    colors[3] = SDL_MapRGB(screenSurface->format, 0x11, 0x56, 0xFF);
#endif

    SDL_Rect whole = {0,0,640,480};
    hud = basic_hud_new();

    panel = side_panel_new(-1, -1);
    SDL_Rect sprect = {0,0, BASE_GAUGE(panel)->w,BASE_GAUGE(panel)->h};

    TerrainViewer *viewer;
    viewer = terrain_viewer_new();


    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;
    Uint32 acc = 0;
    i = 3;

#if defined(USE_FGCONN)
    FlightgearConnector *fglink;
    fglink = flightgear_connector_new(6789);
    flightgear_connector_set_nonblocking(fglink);
    FlightgearPacket packet;
#elif defined(USE_FGTAPE)
    FGTape *tape;
    FGTapeSignal signals[15];
    TapeRecord record;
    int found;

    tape = fg_tape_new_from_file("fg-io/fg-tape/dr400.fgtape");
//    fg_tape_dump(tape);
//    exit(0);
    found = fg_tape_get_signals(tape, signals,
        "/position[0]/latitude-deg[0]",
        "/position[0]/longitude-deg[0]",
        "/position[0]/altitude-ft[0]",
        "/orientation[0]/roll-deg[0]",
        "/orientation[0]/pitch-deg[0]",
        "/orientation[0]/heading-deg[0]",
        "/velocities[0]/airspeed-kt[0]",
	    "/velocities[0]/vertical-speed-fps[0]",
        "/engines[0]/engine[0]/rpm[0]",
        "/engines[0]/engine[0]/fuel-flow-gph[0]",
        "/engines[0]/engine[0]/oil-temperature-degf[0]",
        "/engines[0]/engine[0]/oil-pressure-psi[0]",
        "/engines[0]/engine[0]/cht-degf[0]",
        "/engines[0]/engine[0]/fuel-px-psi[0]",
        "/consumables[0]/fuel[0]/tank[0]/level-gal_us[0]",
        NULL
    );
    printf("TapeRecord: found %d out of %d signals\n",found, 15);

    int start_pos = 120; /*Starting position in the tape*/
//    start_pos = 0;

#endif

    Uint32 startms, dtms, last_dtms;
    Uint32 nframes = 0;

    startms = SDL_GetTicks();
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;
        dtms = ticks - startms + (start_pos * 1000.0);

        done = handle_events(elapsed);

#if defined(USE_FGCONN)
        if(flightgear_connector_get_packet(fglink, &packet)){
            basic_hud_set(hud,  5,
                ALTITUDE, (float)packet.altitude,
                AIRSPEED, (float)packet.airspeed,
                VERTICAL_SPEED, packet.vertical_speed,
                PITCH, packet.pitch,
                ROLL, packet.roll
            );
        }
#elif defined(USE_FGTAPE)
        if(dtms - last_dtms >= (1000/25)){ //One update per 1/25 second
            fg_tape_get_data_at(tape, dtms / 1000.0, 15, signals, &record);
            last_dtms = dtms;
            basic_hud_set(hud,  6,
                ALTITUDE, (double)record.altitude,
                AIRSPEED, (double)record.airspeed,
            VERTICAL_SPEED, record.vertical_speed * 60, /*Convert fps to fpm*/
                PITCH, (double)record.pitch,
                ROLL, (double)record.roll,
                HEADING, (double)record.heading
            );
            side_panel_set_rpm(panel, record.rpm);
            side_panel_set_fuel_flow(panel, record.fuel_flow);
            side_panel_set_oil_temp(panel, record.oil_temp);
            side_panel_set_oil_press(panel, record.oil_press);
            side_panel_set_cht(panel, record.cht);
            side_panel_set_fuel_px(panel, record.fuel_px);
            side_panel_set_fuel_qty(panel, record.fuel_qty);

            float lon = fmod(record.longitude+180, 360.0) - 180;
            record.altitude = record.altitude/3.281;
            terrain_viewer_update_plane(viewer,
                record.latitude, record.longitude, record.altitude + 2,
                record.roll, record.pitch, record.heading
            );
            if(last_ticks == 0){ //Do an invisible frame to trigger preload
                GPU_FlushBlitBuffer(); /*begin 3*/
                terrain_viewer_frame(viewer);
                GPU_ResetRendererState(); /*end 3d*/
            }
        }
#endif

#if USE_SDL_GPU
        GPU_ClearRGB(gpu_screen, 0x11, 0x56, 0xFF);
#else
        SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
#endif
        if(g_show3d){
            GPU_FlushBlitBuffer(); /*begin 3*/
            terrain_viewer_frame(viewer);
            GPU_ResetRendererState(); /*end 3d*/
        }

        base_gauge_render(BASE_GAUGE(hud), elapsed, rtarget, &whole);
        base_gauge_render(BASE_GAUGE(panel), elapsed, rtarget, &sprect);
#if USE_SDL_GPU
		GPU_Flip(gpu_screen);
#else
        SDL_UpdateWindowSurface(window);
#endif
        nframes++;
        acc += elapsed;
        if(elapsed < 20){
            SDL_Delay(20 - elapsed);
        }
#if 0
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
#endif
        if(acc >= 1000){ /*1sec*/
            int h,m,s;

            h = dtms/3600000;
            dtms -= dtms/3600000 * h;
            m = dtms / 60000;
            dtms -= 60000 * m;
            s = dtms / 1000;

            printf("%02d:%02d:%02d Current FPS: %03d\r",h,m,s, (1000*nframes)/elapsed);
            fflush(stdout);
            nframes = 0;
            acc = 0;
        }
        i %= N_COLORS;
        last_ticks = ticks;
    }while(!done);

    basic_hud_free(hud);
    side_panel_free(panel);
#if defined(USE_FGCONN)
    flightgear_connector_free(fglink);
#elif defined(USE_FGTAPE)
    fg_tape_free(tape);
#endif
    resource_manager_shutdown();
#if USE_SDL_GPU
	GPU_Quit();
#else
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;

}

