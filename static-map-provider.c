/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "static-map-provider.h"
#include "http-download.h"
#include "logger.h"

static bool static_map_provider_read_config(StaticMapProvider *self);
static const char *static_map_provider_url_template_set(StaticMapProviderUrlTemplate *self,
                                          uint8_t level,
                                          int32_t x, int32_t y);

static GenericLayer *static_map_provider_get_tile(StaticMapProvider *self, uintf8_t level, int32_t x, int32_t y);
static StaticMapProvider *static_map_provider_dispose(StaticMapProvider *self);
static MapProviderOps static_map_provider_ops = {
    .get_tile = (MapProviderGetTileFunc)static_map_provider_get_tile,
    .dispose = (MapProviderDisposeFunc)static_map_provider_dispose
};

StaticMapProvider *static_map_provider_new(const char *home, const char *format,
                                       intf8_t priority)
{
    StaticMapProvider *self;

    self = calloc(1, sizeof(StaticMapProvider));
    if(self){
        if(!static_map_provider_init(self, home, format, priority))
            return (StaticMapProvider*)map_provider_free(MAP_PROVIDER(self));
    }
    return self;
}

StaticMapProvider *static_map_provider_init(StaticMapProvider *self, const char *home,
                                        const char *format, intf8_t priority)
{
    map_provider_init(
        MAP_PROVIDER(self),
        &static_map_provider_ops,
        priority
    );

    self->home = strdup(home);
    if(!self->home) return NULL;

    self->format = strdup(format);
    if(!self->format) return NULL;

    /* format is HOME/LL/XXXXXXX/YYYYYYY.FORMAT
     * We go up to level 23 which means tileX/Y
     * can reach 8388607 which is 7 digits long
     */
    self->bsize = strlen(self->home)
                 + 1 /*slash*/
                 + 2 /*LL*/
                 + 1 /*slash*/
                 + 7 /*XXXXXXX*/
                 + 1 /*slash*/
                 + 7 /*YYYYYYY*/
                 + 1 /*dot*/
                 + strlen(self->format)
                 + 1; /*null byte*/
    self->buffer = malloc(sizeof(char)*self->bsize);
    if(!self->buffer) return NULL;

    /*The read config can fail and the provider still be
     * usable: no config file, etc.*/
    static_map_provider_read_config(self);

    return self;
}

static StaticMapProvider *static_map_provider_dispose(StaticMapProvider *self)
{
    if(self->home)
        free(self->home);
    if(self->format)
        free(self->format);
    if(self->url.base)
        free(self->url.base);
    if(self->buffer)
        free(self->buffer);
    return self;
}

/**
 * @brief Loads up a GenericLayer from a set of coordinates.
 *
 * Client code is responsible for freeing the layer @see generic_layer_free
 *
 * @param self a StaticMapProvider
 * @param level Zoom level
 * @param x x-coordinate of the tile in the map
 * @param y y-coordinate of the tile in the map
 * @return A GenericLayer pointer or NULL on failure
 */
static GenericLayer *static_map_provider_get_tile(StaticMapProvider *self, uintf8_t level, int32_t x, int32_t y)
{

    if(MAP_PROVIDER(self)->nareas && !map_provider_has_tile(MAP_PROVIDER(self), level, x, y))
        return NULL;

    snprintf(self->buffer, self->bsize, "%s/%d/%d/%d.%s", self->home, level, x, y, self->format);
    if(access(self->buffer, F_OK) != 0){
        /*  This is downloading feature is not intended to make it
         *  into the final version. Maps should be deployed/installed
         *  as a whole (maybe using a grabbing script) and not tile by tile
         *  and certainly not at runtime.
         *
         *  This feature is nonetheless very useful for the dev version
         *  and for demos.
         * */
        if(!self->url.base) return NULL;
        static_map_provider_url_template_set(&self->url, level, x, y);
        if(!http_download_file(self->url.base, self->buffer)){
            return NULL;
        }
    }

    return generic_layer_new_from_file(self->buffer);
}


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
static const char *static_map_provider_url_template_set(StaticMapProviderUrlTemplate *self,
                                          uint8_t level,
                                          int32_t x, int32_t y)
{
    char tmp;

    if(self->is_tms){
        uint32_t tms_maxy = (1 << level) - 1;
        y = tms_maxy - y;
    }

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

/**
 * @brief parse an 'area' config line and fill @param values.
 *
 * @param line: A line, without trailing spaces that begins with 'area:'
 * (without quotes).
 * @param area: A MapProviderArea to fill with parsed values.
 *
 * @return true on success, false otherwise
 */
static bool map_config_read_area(const char *line, MapProviderArea *area)
{
    size_t found;
    char *parts[5];
    const char *area_parts[] = {"level","left","right","top","bottom"};

    found = split_str(line+5, isspace, parts, 5);
    if(found < 5){
    	LOG_ERROR("Missing area part: %s\n", area_parts[found]);
        return false;
    }

    /* TODO: replace atoi() with something that can handle
     * failures.*/
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
static bool map_config_read_url_template(uintf8_t keyword_len, const char *line, StaticMapProviderUrlTemplate *url)
{
    char *tmp;
    int len;

    tmp = nibble_spaces(line+keyword_len, 0);
    if(!tmp) return false;
    url->base = strdup(tmp);
    len = strlen(url->base);
    if(url->base[len-1] == '\n')
        url->base[len-1] = '\0';
    LOG_INFO("Found url: %s", url->base);

    url->lvl = strstr(url->base, "%LEVEL%");
    if(!url->lvl){
    	LOG_ERROR("Missing %%LEVE%% placeholder in url template\n");
        return false;
    }

    url->tilex = strstr(url->base, "%TILE_X%");
    if(!url->tilex){
    	LOG_ERROR("Missing %%TILE_X%% placeholder in url template\n");
        return false;
    }

    url->tiley = strstr(url->base, "%TILE_Y%");
    if(!url->tiley){
    	LOG_ERROR("Missing %%TILE_Y%% placeholder in url template\n");
        return false;
    }
    return true;
}

static bool static_map_provider_read_config(StaticMapProvider *self)
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
    size_t nareas = 0;
    while((read = getline(&line, &aline, fp)) != -1){
        iter = nibble_spaces(line, read);
        if(!iter || *iter == '#' ) continue;

        if(!strncmp(iter, "area:",5)) nareas++;
    }
    /*Then allocate and fill*/
    LOG_INFO("Found %zu level areas", nareas);
    map_provider_set_nareas(MAP_PROVIDER(self), nareas);

    fseek(fp, mark, SEEK_SET);
    size_t current_area = 0;
    while((read = getline(&line, &aline, fp)) != -1){
        iter = nibble_spaces(line, read);
        if(!iter || *iter == '#' ) continue;

        if(MAP_PROVIDER(self)->areas && !strncmp(iter, "area:",5)){
            bool rv;
            rv = map_config_read_area(iter, &MAP_PROVIDER(self)->areas[current_area++]);
            if(!rv) continue; /*TODO: (currently) does nothing*/
        }else if(!strncmp(iter, "src:",4)){
            bool rv;
            rv = map_config_read_url_template(4, iter, &self->url);
            if(!rv) continue; /*TODO: (currently) does nothing*/
        }else if(!strncmp(iter, "src-tms:",8)){
            bool rv;
            rv = map_config_read_url_template(8, iter, &self->url);
            if(!rv) continue;
            self->url.is_tms = true;
        }

    }
    free(line);
    fclose(fp);

    return true;
}
