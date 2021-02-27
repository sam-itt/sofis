#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include "SDL_opengl.h"
#include "basic-hud.h"
#include "side-panel.h"
#include "map-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"

#if ENABLE_3D
#include "terrain-viewer.h"
#endif

#define ENABLE_FGCONN 1
#define ENABLE_FGTAPE 1

#include "data-source.h"
#if ENABLE_FGCONN
#include "fg-data-source.h"
#endif
#if ENABLE_FGTAPE
#include "fg-tape-data-source.h"
#endif

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define N_COLORS 4

typedef enum{
    MODE_FGREMOTE,
    MODE_FGTAPE,
    N_MODES
}RunningMode;

BasicHud *hud = NULL;
SidePanel *panel = NULL;
MapGauge *map = NULL;
bool g_show3d = false;
DataSource *g_ds;
RunningMode g_mode;

/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{
    switch(event->keysym.sym){
        /*App control*/
        case SDLK_ESCAPE:
            if(event->state == SDL_PRESSED)
                return true;
            break;
        case SDLK_SPACE:
            if(event->state == SDL_PRESSED)
                g_show3d = !g_show3d;
            hud->attitude->mode = (g_show3d) ? AI_MODE_3D : AI_MODE_2D;
            break;
        case SDLK_RETURN:
            if(event->state == SDL_PRESSED){
                if(g_mode == MODE_FGTAPE)
                    ((FGTapeDataSource*)(g_ds))->playing = !((FGTapeDataSource*)(g_ds))->playing;
            }
            break;
        case SDLK_p:
            if(event->state == SDL_PRESSED){
                printf("Pitch: %f\nHeading: %f\n",
                    g_ds->pitch,
                    g_ds->heading
                );
            }
            break;

        /*MapGauge controls*/
        case SDLK_UP:
            if(event->state == SDL_PRESSED){
                map_gauge_manipulate_viewport(map, 0, -10, true);
            }
            break;
        case SDLK_DOWN:
            if(event->state == SDL_PRESSED){
                map_gauge_manipulate_viewport(map, 0, 10, true);
            }
            break;
        case SDLK_LEFT:
            if(event->state == SDL_PRESSED){
                map_gauge_manipulate_viewport(map, -10, 0, true);
            }
            break;
        case SDLK_RIGHT:
            if(event->state == SDL_PRESSED){
                map_gauge_manipulate_viewport(map, 10, 0, true);
            }
            break;
        case SDLK_KP_PLUS:
            if(event->state == SDL_PRESSED){
                map_gauge_set_level(map, map->level+1);
            }
            break;
        case SDLK_KP_MINUS:
            if(event->state == SDL_PRESSED){
                map_gauge_set_level(map, map->level-1);
            }
            break;
        case SDLK_KP_DIVIDE:
            if(event->state == SDL_PRESSED){
                map_gauge_center_on_marker(map, true);
            }
            break;

        /*Manual camera/position control*/
        case SDLK_z:
            if(event->state == SDL_PRESSED)
                g_ds->pitch += 1.0;
            break;
        case SDLK_s:
            if(event->state == SDL_PRESSED)
                g_ds->pitch -= 1.0;
            break;
        case SDLK_q:
            if(event->state == SDL_PRESSED)
                g_ds->roll -= 1.0;
            break;
        case SDLK_d:
            if(event->state == SDL_PRESSED)
                g_ds->roll += 1.0;
            break;
        case SDLK_a:
            if(event->state == SDL_PRESSED){
                g_ds->heading -= 1.0;
                g_ds->heading = fmodf(g_ds->heading, 360.0);
            }
            break;
        case SDLK_e:
            if(event->state == SDL_PRESSED){
                g_ds->heading += 1.0;
                g_ds->heading = fmodf(g_ds->heading, 360.0);
            }
            break;
        case SDLK_PAGEUP:
            if(event->state == SDL_PRESSED)
                g_ds->altitude += 10;
            break;
        case SDLK_PAGEDOWN:
            if(event->state == SDL_PRESSED)
                g_ds->altitude -= 10;
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

const char *pretty_mode(RunningMode mode)
{
    switch(mode){
        case MODE_FGREMOTE:
            return "FGDataSource";
            break;
        case MODE_FGTAPE:
            return "FGTapeDataSource";
        default:
            return "Unknown!";
    }

}

int main(int argc, char **argv)
{
    Uint32 colors[N_COLORS];
    bool done;
    int i;
    float oldv[5] = {0,0,0,0,0};
    RenderTarget rtarget;

    g_mode = MODE_FGTAPE;
    if(argc > 1){
        if(!strcmp(argv[1], "--fgtape"))
            g_mode = MODE_FGTAPE;
        else if(!strcmp(argv[1], "--fgremote"))
            g_mode = MODE_FGREMOTE;
    }

    switch(g_mode){
        case MODE_FGREMOTE:
            g_ds = (DataSource *)fg_data_source_new(6798);
            break;
        case MODE_FGTAPE: //Fallthtough
        default:
            g_ds = (DataSource *)fg_tape_data_source_new("fg-io/fg-tape/dr400.fgtape", 120);
            break;
    }

    if(!g_ds){
        printf("Couldn't create DataSource (%s), bailing out\n", pretty_mode(g_mode));
        exit(EXIT_FAILURE);
    }

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
    SDL_Rect sprect = {0,0, base_gauge_w(BASE_GAUGE(panel)),base_gauge_h(BASE_GAUGE(panel))};

    map = map_gauge_new(190,150);
    map->level = 7;
    SDL_Rect maprect = {SCREEN_WIDTH-200,SCREEN_HEIGHT-160,base_gauge_w(BASE_GAUGE(map)),base_gauge_h(BASE_GAUGE(map))};

#if ENABLE_3D
    TerrainViewer *viewer;
    viewer = terrain_viewer_new(-0.2);
#endif

    done = false;
    Uint32 ticks;
    Uint32 last_ticks = 0;
    Uint32 elapsed = 0;
    Uint32 acc = 0;
    i = 3;

    Uint32 startms, dtms, last_dtms;
    Uint32 nframes = 0;
    Uint32 render_start, render_end;
    Uint32 total_render_time = 0;
    Uint32 nrender_calls = 0;
#if ENABLE_3D
    g_show3d = true;
#endif
    hud->attitude->mode = (g_show3d) ? AI_MODE_3D : AI_MODE_2D;

    startms = SDL_GetTicks();
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;
        dtms = ticks - startms;

        done = handle_events(elapsed);

        if(data_source_frame(DATA_SOURCE(g_ds), dtms - last_dtms)){
            last_dtms = dtms;
            basic_hud_set(hud,  7,
                ALTITUDE, (double)DATA_SOURCE(g_ds)->altitude,
                AIRSPEED, (double)DATA_SOURCE(g_ds)->airspeed,
                VERTICAL_SPEED, (double)DATA_SOURCE(g_ds)->vertical_speed * 60, /*Convert fps to fpm*/
                PITCH, (double)DATA_SOURCE(g_ds)->pitch,
                ROLL, (double)DATA_SOURCE(g_ds)->roll,
                HEADING, (double)DATA_SOURCE(g_ds)->heading,
                SLIP, (double)DATA_SOURCE(g_ds)->slip_rad* 180.0/M_PI
            );
            side_panel_set_rpm(panel, DATA_SOURCE(g_ds)->rpm);
            side_panel_set_fuel_flow(panel, DATA_SOURCE(g_ds)->fuel_flow);
            side_panel_set_oil_temp(panel, DATA_SOURCE(g_ds)->oil_temp);
            side_panel_set_oil_press(panel, DATA_SOURCE(g_ds)->oil_press);
            side_panel_set_cht(panel, DATA_SOURCE(g_ds)->cht);
            side_panel_set_fuel_px(panel, DATA_SOURCE(g_ds)->fuel_px);
            side_panel_set_fuel_qty(panel, DATA_SOURCE(g_ds)->fuel_qty);

            map_gauge_set_marker_position(map, DATA_SOURCE(g_ds)->latitude, DATA_SOURCE(g_ds)->longitude);
            map_gauge_set_marker_heading(map, DATA_SOURCE(g_ds)->heading);

#if ENABLE_3D
            float lon = fmod(DATA_SOURCE(g_ds)->longitude+180, 360.0) - 180;
            terrain_viewer_update_plane(viewer,
                DATA_SOURCE(g_ds)->latitude, DATA_SOURCE(g_ds)->longitude, DATA_SOURCE(g_ds)->altitude/3.281 + 2,
                DATA_SOURCE(g_ds)->roll, DATA_SOURCE(g_ds)->pitch, DATA_SOURCE(g_ds)->heading
            );
            if(last_ticks == 0){ //Do an invisible frame to trigger preload
                GPU_FlushBlitBuffer(); /*begin 3*/
                terrain_viewer_frame(viewer);
                GPU_ResetRendererState(); /*end 3d*/
            }
#endif
        }
#if USE_SDL_GPU
        GPU_ClearRGB(gpu_screen, 0x11, 0x56, 0xFF);
#else
        SDL_FillRect(screenSurface, NULL, SDL_UFBLUE(screenSurface));
#endif
#if ENABLE_3D
        if(g_show3d){
            GPU_FlushBlitBuffer(); /*begin 3*/
            glDisable(GL_BLEND);
            terrain_viewer_frame(viewer);
            GPU_ResetRendererState(); /*end 3d*/
        }
#endif
        render_start = SDL_GetTicks();
        base_gauge_render(BASE_GAUGE(hud), elapsed, &(RenderContext){rtarget, &whole, NULL});
        base_gauge_render(BASE_GAUGE(panel), elapsed, &(RenderContext){rtarget, &sprect, NULL});
        base_gauge_render(BASE_GAUGE(map), elapsed, &(RenderContext){rtarget, &maprect, NULL});
        render_end = SDL_GetTicks();
        total_render_time += render_end - render_start;
        nrender_calls++;

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

    printf("Average rendering time (%d samples): %f ticks\n", nrender_calls, total_render_time*1.0/nrender_calls);
    basic_hud_free(hud);
    side_panel_free(panel);
    map_gauge_free(map);
    data_source_free(DATA_SOURCE(g_ds));
    resource_manager_shutdown();
#if USE_SDL_GPU
	GPU_Quit();
#else
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;
}
