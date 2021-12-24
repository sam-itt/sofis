#ifndef ROUTE_MAP_PROVIDER_H
#define ROUTE_MAP_PROVIDER_H
#include "geo-location.h"
#include "map-provider.h"
#include "misc.h"

typedef struct{
    MapProvider super;

    /*Lat/lon*/
    GeoLocation geo_from;
    GeoLocation geo_to;

    intf8_t current_zoom;
    SDL_Point from;
    SDL_Point to;
}RouteMapProvider;

RouteMapProvider *route_map_provider_new(void);
RouteMapProvider *route_map_provider_init(RouteMapProvider *self);

bool route_map_provider_set_route(RouteMapProvider *self,
                                  GeoLocation *from,
                                  GeoLocation *to);

#endif /* ROUTE_MAP_PROVIDER_H */
