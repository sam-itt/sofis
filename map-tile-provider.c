#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "map-tile-provider.h"
#include "http-download.h"
#include "misc.h"

static bool map_tile_provider_read_config(MapTileProvider *self);
static bool map_tile_provider_has_tile(MapTileProvider *self, uintf8_t level, uint32_t x, uint32_t y);

/**
 * @brief Creates a fetching URL for a given tile.
 *
 * The caller should not free the returned value. Any subsequent call
 * will alter the pointed value. Use a copy if you want the value to survive
 * the next call. Be cautious when using threads. Only one thread should call
 * it and distribute copies to others.
 *
 * @param self The MapProviderUrlTemplate
 * @param level the level
 * @parm x The x coordinates of the tile within level @param level
 * @parm y The y coordinates of the tile within level @param level
 * @return The url that can be used to fetch the tile.
 */
static const char *map_provider_url_template_set(MapProviderUrlTemplate *self,
                                          uint8_t level,
                                          uint32_t x, uint32_t y)
{
    char tmp;

    /* snprintf behavior regarding size and the null byte is
     * implementation-defined. On Linux it will always null-terminate
     * the string, therefore we print the full size and we save/restore
     * the char just after the place holder*/
    tmp = self->lvl[7]; /*byte index 7 is the 8th*/
    /* %LEVEL% is 7 bytes so we need to print seven digits to override it fully
     * size is 8 to output a null byte *after* the level. If it was 7 and strncpy
     * impl wants to null-terminate the string (as in Linux) it would break the number
     * instead of stopping after the last digit and omitting the null byte
     * */
    snprintf(self->lvl, 8, "%07d", level);
    self->lvl[7] = tmp;

    tmp = self->tilex[8];/*byte index 8 is the 9th*/
    /*%TILE_X% is 8 bytes long*/
    snprintf(self->tilex, 9, "%08d", x);
    self->tilex[8] = tmp;

    tmp = self->tiley[8];
    /*%TILE_X% is 8 bytes long*/
    snprintf(self->tiley, 9, "%08d", y);
    self->tiley[8] = tmp;

    return self->base;
}

#if 0
static inline bool map_tile_provider_area_contains(MapTileProviderArea)
/*inline or macro*/
uint8_t rect_contains(Rect *self, int x, int y)
{
    return ((x >= self->left) && (x <= self->right) && (y >= self->top) && (y <= self->bottom));
}
#endif

MapTileProvider *map_tile_provider_new(const char *home, const char *format)
{
    MapTileProvider *self;

    self = calloc(1, sizeof(MapTileProvider));
    if(self){
        if(!map_tile_provider_init(self, home, format))
            return map_tile_provider_free(self);
    }
    return self;
}

MapTileProvider *map_tile_provider_init(MapTileProvider *self, const char *home,
                                        const char *format)
{
    self->home = strdup(home);
    if(!self->home) return NULL;

    self->format = strdup(format);
    if(!self->format) return NULL;

    /*The read config can fail and the provider still be
     * usable: no config file, etc.*/
    map_tile_provider_read_config(self);

    return self;
}

MapTileProvider *map_tile_provider_dispose(MapTileProvider *self)
{
    if(self->home)
        free(self->home);
    if(self->format)
        free(self->format);
    if(self->url.base)
        free(self->url.base);
    if(self->areas)
        free(self->areas);
    return self;
}

MapTileProvider *map_tile_provider_free(MapTileProvider *self)
{
    map_tile_provider_dispose(self);
    free(self);
    return NULL;
}

/**
 * @brief Loads up a GenericLayer from a set of coordinates.
 *
 * Client code is responsible for freeing the layer @see generic_layer_free
 *
 * @param self a MapTileProvider
 * @param level Zoom level
 * @param x x-coordinate of the tile in the map
 * @param y y-coordinate of the tile in the map
 * @return A GenericLayer pointer or NULL on failure
 */
GenericLayer *map_tile_provider_get_tile(MapTileProvider *self, uintf8_t level, uint32_t x, uint32_t y)
{
    char *filename;
    GenericLayer *rv;

    if(self->nareas && !map_tile_provider_has_tile(self, level, x, y))
        return NULL;

    asprintf(&filename, "%s/%d/%d/%d.%s", self->home, level, x, y, self->format);
    if(access(filename, F_OK) != 0){
        /*  This is downloading feature is not intended to make it
         *  into the final version. Maps should be deployed/installed
         *  as a whole (maybe using a grabbing script) and not tile by tile
         *  and certainly not at runtime.
         *
         *  This feature is nonetheless very useful for the dev version
         *  and for demos.
         * */
        if(!self->url.base) goto out;
        map_provider_url_template_set(&self->url, level, x, y);
        if(!http_download_file(self->url.base, filename)){
            goto out;
        }
    }

    rv = generic_layer_new_from_file(filename);
out:
    free(filename);
    return rv;
}

static bool map_tile_provider_has_tile(MapTileProvider *self, uintf8_t level, uint32_t x, uint32_t y)
{
    int i;

    for(i = 0; i < self->nareas; i++){
        if(self->areas[i].level == level){
            return(   (x >= self->areas[i].left) && (x <= self->areas[i].right)
                   && (y >= self->areas[i].top)  && (y <= self->areas[i].bottom)
            );
        }
    }

    /*true if no area is registered, false if there is any and we got here*/
    return (i == 0);
}



/**
 * @brief parse an 'area' config line and fill @param values.
 *
 * @param line: A line, without trailing spaces that begins with 'area:'
 * (without quotes).
 * @param area: A MapTileProviderArea to fill with parsed values.
 *
 * @return true on success, false otherwise
 * TODO: Current implementation using atoi() restrics to level 15
 */
static bool map_config_read_area(const char *line, MapTileProviderArea *area)
{
    size_t found;
    char *parts[5];
    const char *area_parts[] = {"level","left","right","top","bottom"};

    found = split_str(line+5, isspace, parts, 5);
    if(found < 5){
        printf("Missing area part: %s\n", area_parts[found]);
        return false;
    }

    /* TODO: replace atoi() with something that can handle
     * unsigned ints full range, and failures.*/
    area->level = atoi(parts[0]);
    area->left = atoi(parts[1]);
    area->right = atoi(parts[2]);
    area->top = atoi(parts[3]);
    area->bottom = atoi(parts[4]);
    return true;
}

/**
 * @brief parse a 'src' config line and turn it into a MapProviderUrlTemplate.
 *
 * @param line: A line, without trailing spaces that begins with 'src:'
 * (without quotes).
 * @param url: Address to a MapProviderUrlTemplate to be inited.
 *
 * @return true on success, false otherwise
 */
static bool map_config_read_url_template(const char *line, MapProviderUrlTemplate *url)
{
    char *tmp;
    size_t read;
    int len;

    tmp = nibble_spaces(line+4, read);
    if(!tmp) return false;
    url->base = strdup(tmp);
    len = strlen(url->base);
    if(url->base[len-1] == '\n')
        url->base[len-1] = '\0';
    printf("Found url: %s\n", url->base);

    url->lvl = strstr(url->base, "%LEVEL%");
    if(!url->lvl){
        printf("Missing %%LEVE%% placeholder in url template\n");
        return false;
    }

    url->tilex = strstr(url->base, "%TILE_X%");
    if(!url->tilex){
        printf("Missing %%TILE_X%% placeholder in url template\n");
        return false;
    }

    url->tiley = strstr(url->base, "%TILE_Y%");
    if(!url->tiley){
        printf("Missing %%TILE_Y%% placeholder in url template\n");
        return false;
    }
    return true;
}

static bool map_tile_provider_read_config(MapTileProvider *self)
{
    char *filename;
    size_t flen;
    FILE *fp;

    char *line = NULL;
    size_t aline;
    size_t read;

    /* returns the number of characters that would have been printed
     * *excluding* the null byte */
    flen = snprintf(NULL, 0, "%s/map.conf", self->home);
    /*TODO: add malloca/freea to revert to the heap when size is too big (1KB?)*/
    filename = alloca((flen+1)*sizeof(char));
    /*size must include the null byte if we don't want the string to be truncated*/
    snprintf(filename, flen+1, "%s/map.conf", self->home);

    fp = fopen(filename,"r");
    if(!fp)
        return false;

    size_t found;

    char *iter;
    /*First, count the number of areas/levels bounds described in the file*/
    off_t mark = ftell(fp);
    while((read = getline(&line, &aline, fp)) != -1){
        iter = nibble_spaces(line, read);
        if(!iter || *iter == '#' ) continue;

        if(!strncmp(iter, "area:",5)) self->nareas++;
    }
    /*Then allocate and fill*/
    printf("Found %d level areas\n", self->nareas);
    if(self->nareas)
        self->areas = calloc(self->nareas, sizeof(MapTileProviderArea));

    fseek(fp, mark, SEEK_SET);
    size_t current_area = 0;
    while((read = getline(&line, &aline, fp)) != -1){
        iter = nibble_spaces(line, read);
        if(!iter || *iter == '#' ) continue;

        if(self->areas && !strncmp(iter, "area:",5)){
            bool rv;
            rv = map_config_read_area(iter, &self->areas[current_area++]);
            if(!rv) continue; /*TODO: (currently) does nothing*/
        }else if(!strncmp(iter, "src:",4)){
            bool rv;
            rv = map_config_read_url_template(iter, &self->url);
            if(!rv) continue; /*TODO: (currently) does nothing*/
        }
    }
    free(line);
    fclose(fp);

    return true;
}
