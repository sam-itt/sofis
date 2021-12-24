#ifndef STATIC_MAP_TILE_PROVIDER_H
#define STATIC_MAP_TILE_PROVIDER_H
#include "map-tile-provider.h"

typedef struct{
    /* In TMS mode, the Y axis (tiles coordinates within the world map)
     * is reversed*/
    bool is_tms;
    char *base;

    char *lvl;
    char *tilex;
    char *tiley;
}StaticMapTileProviderUrlTemplate;


typedef struct{
    MapTileProvider super;

    char *home;
    char *format; /*tile file extension*/
    StaticMapTileProviderUrlTemplate url;
}StaticMapTileProvider;

StaticMapTileProvider *static_map_tile_provider_new(const char *home, const char *format, intf8_t priority);

StaticMapTileProvider *static_map_tile_provider_init(StaticMapTileProvider *self, const char *home,
                                        const char *format, intf8_t priority);

#endif /* STATIC_MAP_TILE_PROVIDER_H */
