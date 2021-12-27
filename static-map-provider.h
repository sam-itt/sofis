#ifndef STATIC_MAP_PROVIDER_H
#define STATIC_MAP_PROVIDER_H
#include "map-provider.h"

typedef struct{
    /* In TMS mode, the Y axis (tiles coordinates within the world map)
     * is reversed*/
    bool is_tms;
    char *base;

    char *lvl;
    char *tilex;
    char *tiley;
}StaticMapProviderUrlTemplate;


typedef struct{
    MapProvider super;

    char *home;
    char *format; /*tile file extension*/
    char *buffer; /*store filenames*/
    size_t bsize; /*in bytes*/
    StaticMapProviderUrlTemplate url;
}StaticMapProvider;

StaticMapProvider *static_map_provider_new(const char *home, const char *format, intf8_t priority);

StaticMapProvider *static_map_provider_init(StaticMapProvider *self, const char *home,
                                            const char *format, intf8_t priority);

#endif /* STATIC_MAP_PROVIDER_H */
