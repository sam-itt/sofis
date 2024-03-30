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

#include "base-gauge.h"
#include "basic-hud.h"
#include "dialogs/direct-to-dialog.h"
#include "side-panel.h"
#include "map-gauge.h"
#include "resource-manager.h"
#include "sdl-colors.h"
#include "widgets/base-widget.h"
#include "logger.h"

#if ENABLE_3D
#include "terrain-viewer.h"
#endif

#define ENABLE_FGCONN 1
#define ENABLE_FGTAPE 1
#define ENABLE_SENSORS 1
#define ENABLE_STRATUX 1
#define ENABLE_MOCK 1

#include "data-source.h"
#if ENABLE_FGCONN
#include "fg-data-source.h"
#endif
#if ENABLE_FGTAPE
#include "fg-tape-data-source.h"
#endif
#if ENABLE_SENSORS
#include "sensors-data-source.h"
#endif
#if ENABLE_STRATUX
#include "stratux-data-source.h"
#endif
#if ENABLE_MOCK
#include "mock-data-source.h"
#endif

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define N_COLORS 4

typedef enum{
    MODE_FGREMOTE,
    MODE_FGTAPE,
    MODE_SENSORS,
    MODE_STRATUX,
    MODE_MOCK,
    N_MODES
}RunningMode;

BasicHud *hud = NULL;
SidePanel *panel = NULL;
MapGauge *map = NULL;
DirectToDialog *ddt = NULL;

bool g_show3d = false;
DataSource *g_ds;
RunningMode g_mode;

/*Return true to quit the app*/
bool handle_keyboard(SDL_KeyboardEvent *event, Uint32 elapsed)
{
    AttitudeData new_attitude;
    LocationData new_location;
    bool attitude_dirty = false;
    bool location_dirty = false;

    new_attitude = g_ds->attitude;
    new_location = g_ds->location;

    if(ddt && ddt->visible)
        base_widget_handle_event(BASE_WIDGET(ddt), event);

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
                LOG_INFO("Pitch: %f\nHeading: %f\n",
                    g_ds->attitude.pitch,
                    g_ds->attitude.heading
                );
            }
            break;

        /*MapGauge controls*/
        case SDLK_KP_8: /*keypad up arrows*/
            if(event->state == SDL_PRESSED){
                map_gauge_manipulate_viewport(map, 0, -10, true);
            }
            break;
        case SDLK_KP_2: /*keypad down arrows*/
            if(event->state == SDL_PRESSED){
                map_gauge_manipulate_viewport(map, 0, 10, true);
            }
            break;
        case SDLK_KP_4: /*keypad left arrows*/
            if(event->state == SDL_PRESSED){
                map_gauge_manipulate_viewport(map, -10, 0, true);
            }
            break;
        case SDLK_KP_6: /*keypad right arrows*/
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

        /*Go to dialog*/
        case SDLK_g:
            if(event->state == SDL_PRESSED){
                if(!ddt)
                    ddt = direct_to_dialog_new();
                else
                    direct_to_dialog_reset(ddt);
                ddt->visible = true;
            }
            break;

        /*Manual camera/position control*/
        case SDLK_z:
            if(event->state == SDL_PRESSED){
                new_attitude.pitch += 1.0;
                attitude_dirty = true;
            }
            break;
        case SDLK_s:
            if(event->state == SDL_PRESSED){
                new_attitude.pitch -= 1.0;
                attitude_dirty = true;
            }
            break;
        case SDLK_q:
            if(event->state == SDL_PRESSED){
                new_attitude.roll -= 1.0;
                attitude_dirty = true;
            }
            break;
        case SDLK_d:
            if(event->state == SDL_PRESSED){
                new_attitude.roll += 1.0;
                attitude_dirty = true;
            }
            break;
        case SDLK_a:
            if(event->state == SDL_PRESSED){
                new_attitude.heading -= 1.0;
                new_attitude.heading = fmodf(new_attitude.heading, 360.0);
                attitude_dirty = true;
            }
            break;
        case SDLK_e:
            if(event->state == SDL_PRESSED){
                new_attitude.heading += 1.0;
                new_attitude.heading = fmodf(new_attitude.heading, 360.0);
                attitude_dirty = true;
            }
            break;
        case SDLK_PAGEUP:
            if(event->state == SDL_PRESSED){
                new_location.altitude += 10;
                attitude_dirty = true;
            }
            break;
        case SDLK_PAGEDOWN:
            if(event->state == SDL_PRESSED){
                new_location.altitude -= 10;
                attitude_dirty = true;
            }
            break;
    }
    if(attitude_dirty)
        data_source_set_attitude(g_ds, &new_attitude);
    if(location_dirty)
        data_source_set_location(g_ds, &new_location);
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
        case MODE_FGTAPE:
            return "FGTapeDataSource";
        case MODE_SENSORS:
            return "SensorsDataSource";
        case MODE_STRATUX:
            return "StratuxDataSource";
        case MODE_MOCK:
            return "MockDataSource";

        default:
            return "Unknown!";
    }

}

#if ENABLE_3D
void update_terrain_viewer_location(TerrainViewer *self, LocationData *newv)
{
	LOG_TRACE("Entering update_terrain_viewer_location");
    if(self == NULL || newv == NULL) {
    	 LOG_ERROR("update_terrain_viewer_location: Received NULL pointer (self: %p, newv: %p)", self, newv);
         return;
    }

    double lon = fmod(newv->super.longitude+180, 360.0) - 180;
    plane_set_position(self->plane, newv->super.latitude, lon, newv->altitude/3.281 + 2);
    self->dirty = true;
    LOG_TRACE("Updated terrain viewer location: Lat: %f, Lon: %f, Alt: %f", newv->super.latitude, lon, newv->altitude);

}

void update_terrain_viewer_attitude(TerrainViewer *self, AttitudeData *newv)
{
    if(self == NULL || newv == NULL) {
    	 LOG_ERROR("update_terrain_viewer_attitude: Received NULL pointer (self: %p, newv: %p)", self, newv);
         return;
    }

    plane_set_attitude(self->plane, newv->roll, newv->pitch, newv->heading);
    LOG_TRACE("Updated terrain viewer attitude: Roll: %f, Pitch: %f, Heading: %f", newv->roll, newv->pitch, newv->heading);
    self->dirty = true;
}
#endif

int main(int argc, char **argv)
{
	/*stdout*/
	logger_initConsoleLogger(NULL);
	logger_setLevel(LogLevel_DEBUG);
	LOG_DEBUG("Application start");
    Uint32 colors[N_COLORS];
    bool done;
    int i;
    float oldv[5] = {0,0,0,0,0};
    RenderTarget rtarget;
    LOG_DEBUG("Default mode set to MODE_FGTAPE");
    g_mode = MODE_FGTAPE;
    if(argc > 1){
    	LOG_DEBUG("Processing command line arguments");
        if(!strcmp(argv[1], "--sensors"))
            g_mode = MODE_SENSORS;
        else if(!strcmp(argv[1], "--fgtape"))
            g_mode = MODE_FGTAPE;
        else if(!strcmp(argv[1], "--fgremote"))
            g_mode = MODE_FGREMOTE;
        else if(!strcmp(argv[1], "--stratux"))
            g_mode = MODE_STRATUX;
        else if(!strcmp(argv[1], "--mock"))
            g_mode = MODE_MOCK;
        LOG_INFO("Running mode set via command line: %s", pretty_mode(g_mode));
    }
    LOG_DEBUG("Initializing data source based on selected mode");
    switch(g_mode){
        case MODE_SENSORS:
            g_ds = (DataSource *)sensors_data_source_new();
            LOG_DEBUG("MODE_SENSORS data source initialized");
            break;
        case MODE_FGREMOTE:
            g_ds = (DataSource *)fg_data_source_new(6789);
            LOG_DEBUG("MODE_FGREMOTE data source initialized");
            break;
        case MODE_STRATUX:
            g_ds = (DataSource *)stratux_data_source_new();
        	LOG_DEBUG("MODE_STRATUX data source initialized");
            break;
        case MODE_MOCK:
            g_ds = (DataSource*)mock_data_source_new();
            LOG_DEBUG("MODE_MOCK data source initialized");
            break;
        case MODE_FGTAPE: //Fallthtough
        default:
            g_ds = (DataSource *)fg_tape_data_source_new("fg-io/fg-tape/dr400.fgtape", 120);
            LOG_DEBUG("Default data source (MODE_FGTAPE) initialized");
            break;
    }

    if(!g_ds){
    	LOG_FATAL("Couldn't create DataSource (%s), bailing out", pretty_mode(g_mode));
        exit(EXIT_FAILURE);
    }
    data_source_set(g_ds);
    LOG_DEBUG("Data source set successfully");

#if USE_SDL_GPU
    GPU_Target* gpu_screen = NULL;

	GPU_SetRequiredFeatures(GPU_FEATURE_BASIC_SHADERS);
#if USE_GLES
	gpu_screen = GPU_InitRenderer(GPU_RENDERER_GLES_2, SCREEN_WIDTH, SCREEN_HEIGHT, GPU_DEFAULT_INIT_FLAGS);
#else
	gpu_screen = GPU_InitRenderer(GPU_RENDERER_OPENGL_2, SCREEN_WIDTH, SCREEN_HEIGHT, GPU_DEFAULT_INIT_FLAGS);
#endif
	if(gpu_screen == NULL){
		LOG_FATAL("Initialization Error: Could not create a renderer with proper feature support for this demo.\n");
		return 1;
    }
    rtarget.target = gpu_screen;
#else
    SDL_Window* window = NULL;
    SDL_Surface* screenSurface = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    	LOG_FATAL("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 1;
    } else {
        LOG_DEBUG("SDL initialized successfully.");
    }

    window = SDL_CreateWindow(
                "HUD testbench",
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                SCREEN_WIDTH, SCREEN_HEIGHT,
                SDL_WINDOW_SHOWN
                );
    if (window == NULL) {
        LOG_FATAL("could not create window: %s\n", SDL_GetError());
        return 1;
    }

    screenSurface = SDL_GetWindowSurface(window);
    if(!screenSurface){
        LOG_FATAL("Error: %s\n",SDL_GetError());
        exit(-1);
    }
    rtarget.surface = screenSurface;

    colors[0] = SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF);
    colors[1] = SDL_MapRGB(screenSurface->format, 0xFF, 0x00, 0x00);
    colors[2] = SDL_MapRGB(screenSurface->format, 0x00, 0xFF, 0x00);
    colors[3] = SDL_MapRGB(screenSurface->format, 0x11, 0x56, 0xFF);
#endif
    SDL_ShowCursor(SDL_DISABLE);

    SDL_Rect whole = {0,0,640,480};
    hud = basic_hud_new();

    panel = side_panel_new(-1, -1);
    SDL_Rect sprect = {0,0, base_gauge_w(BASE_GAUGE(panel)),base_gauge_h(BASE_GAUGE(panel))};

    map = map_gauge_new(190,150);
    map->level = 7;
    SDL_Rect maprect = {SCREEN_WIDTH-200,SCREEN_HEIGHT-160,base_gauge_w(BASE_GAUGE(map)),base_gauge_h(BASE_GAUGE(map))};

    SDL_Rect ddtrect ={
        640/2,
        480/2 - 100,
        12*20,
        304
    };

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

    if(g_mode == MODE_FGREMOTE)
        fg_data_source_banner((FGDataSource*)g_ds);

    data_source_add_events_listener(g_ds, hud, 3,
        ATTITUDE_DATA, basic_hud_attitude_changed,
        DYNAMICS_DATA, basic_hud_dynamics_changed,
        LOCATION_DATA, basic_hud_location_changed
    );

    data_source_add_listener(g_ds, ENGINE_DATA, &(ValueListener){
        .callback = (ValueListenerFunc)side_panel_engine_data_changed,
        .target = panel
    });

    data_source_add_events_listener(g_ds, map, 3,
        LOCATION_DATA, map_gauge_location_changed,
        ATTITUDE_DATA, map_gauge_attitude_changed,
        ROUTE_DATA, map_gauge_route_changed
    );

#if ENABLE_3D
    data_source_add_events_listener(g_ds, viewer, 2,
        LOCATION_DATA, update_terrain_viewer_location,
        ATTITUDE_DATA, update_terrain_viewer_attitude
    );
#endif
    data_source_print_listener_stats(g_ds);


    LOG_INFO("Waiting for fix.");
    do{
        data_source_frame(DATA_SOURCE(g_ds), 0);
        sleep(1); /*sleep for 1 sec*/
    }while(!DATA_SOURCE(g_ds)->has_fix);


    last_dtms = 0;
    startms = SDL_GetTicks();
    do{
        ticks = SDL_GetTicks();
        elapsed = ticks - last_ticks;
        dtms = ticks - startms;

        done = handle_events(elapsed);

        if(data_source_frame(DATA_SOURCE(g_ds), dtms - last_dtms)){
            last_dtms = dtms;

#if ENABLE_3D
            if(last_ticks == 0){ //Do an invisible frame to trigger preload
                GPU_FlushBlitBuffer(); /*begin 3*/
                terrain_viewer_frame(viewer);
                GPU_ResetRendererState(); /*end 3d*/
                /*Reset startms as initial loading can take up a while*/
                startms = SDL_GetTicks();
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
            GPU_FlushBlitBuffer(); /*begin 3d*/
            glDisable(GL_BLEND);
            terrain_viewer_frame(viewer);
            GPU_ResetRendererState(); /*end 3d*/
        }
#endif
        render_start = SDL_GetTicks();
        base_gauge_render(BASE_GAUGE(hud), elapsed, &(RenderContext){rtarget, &whole, NULL});
        base_gauge_render(BASE_GAUGE(panel), elapsed, &(RenderContext){rtarget, &sprect, NULL});
        base_gauge_render(BASE_GAUGE(map), elapsed, &(RenderContext){rtarget, &maprect, NULL});
        if(ddt && ddt->visible)
            base_gauge_render(BASE_GAUGE(ddt), elapsed, &(RenderContext){rtarget, &ddtrect, NULL});
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

            LOG_INFO("%02d:%02d:%02d Current FPS: %03d\r",h,m,s, (1000*nframes)/elapsed);
            fflush(stdout);
            nframes = 0;
            acc = 0;
        }
        i %= N_COLORS;
        last_ticks = ticks;
    }while(!done);

    LOG_INFO("Average rendering time (%d samples): %f ticks\n", nrender_calls, total_render_time*1.0/nrender_calls);
    base_gauge_free(BASE_GAUGE(hud));
    base_gauge_free(BASE_GAUGE(panel));
    base_gauge_free(BASE_GAUGE(map));
    data_source_free(DATA_SOURCE(g_ds));
    resource_manager_shutdown();
#if ENABLE_3D
    terrain_viewer_free(viewer);
    texture_store_shutdown();
#endif
#if USE_SDL_GPU
	GPU_Quit();
#else
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;
}
