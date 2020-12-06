#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "generic-layer.h"

#include "SDL_gpu.h"

/**
 * GenericLayer: A class to manage drawing areas (SDL_Surfaces)
 * that will be drawn on (or loaded from file) and then be used as-is
 * without further modification and optionaly turned into textures.
 *
 * This is intended to be used for anything that must be blit to the
 * screen each frame.
 *
 */


/**
 * @brief Creates a new GenericLayer of given size.
 *
 * The GenericLayer returned must be freed by the calling code.
 *
 * @param width canvas width
 * @param height canvas width
 * @return a newly allocated GenericLayer on success, NULL on failure.
 *
 * @see generic_layer_free
 */
GenericLayer *generic_layer_new(int width, int height)
{
   GenericLayer *self;

    self = calloc(1, sizeof(GenericLayer));
    if(self){
        if(!generic_layer_init(self, width, height)){
            generic_layer_dispose(self);
            return NULL;
        }
    }
    return self;
}

/**
 * @brief Creates a new GenericLayer from an existing image. Loading is
 * done through SDL_Image, refer to their documention for format support.
 *
 * The GenericLayer returned must be freed by the calling code.
 *
 * @param filaname The image to load
 * @return a newly allocated GenericLayer on success, NULL on failure.
 *
 * @see generic_layer_free
 */
GenericLayer *generic_layer_new_from_file(const char *filename)
{
    GenericLayer *self;

    self = calloc(1, sizeof(GenericLayer));
    if(self){
        if(!generic_layer_init_from_file(self, filename)){
            generic_layer_dispose(self);
            return NULL;
        }
    }
    return self;
}

/**
 * @brief Creates the underlying canvas (SDL_Surface)
 *
 * After calling this function successfuly, the canvas (self->canvas)
 * can be accessed and drawed on.
 *
 * @param self a GenericLayer
 * @param width the width of the canvas
 * @param height the height of the canvas
 * @return true on success, false otherwise. If the function returns
 * false, the canvas is not suitable for use/access.
 *
 */
bool generic_layer_init(GenericLayer *self, int width, int height)
{
    self->canvas = SDL_CreateRGBSurfaceWithFormat(
        0,
        width, height,
        32, SDL_PIXELFORMAT_RGBA32
    );
/*TODO: Is this useful? Enclosing object would have memeset'ed itself*/
#if USE_SDL_GPU
    self->texture = NULL;
#endif
    return self->canvas != NULL;
}

/**
 * @brief Creates the underlying canvas (SDL_Surface) with the specified mask.
 *
 * After calling this function successfuly, the canvas (self->canvas) can be
 * accessed and drawed on.
 *
 * @note All GenericLayer surfaces will/must have a depth of 32 bits per pixel.
 *
 * @param self a GenericLayer
 * @param width the width of the canvas
 * @param height the height of the canvas
 * @param Rmask the red mask for the pixels
 * @param Gmask the green mask for the pixels
 * @param Bmask the blue mask for the pixels
 * @param Amask the alpha mask for the pixels
 * @return true on success, false otherwise. If the function returns
 * false, the canvas is not suitable for use/access.
 */
bool generic_layer_init_with_masks(GenericLayer *self, int width, int height, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    self->canvas = SDL_CreateRGBSurface(
        0,
        width, height,
        32,
        Rmask, Gmask, Bmask, Amask
    );
/*TODO: Is this useful? Enclosing object would have memeset'ed itself*/
#if USE_SDL_GPU
    self->texture = NULL;
#endif
    return self->canvas != NULL;
}


/**
 * @brief Release the resources held by self.
 *
 * @param self a GenericLayer
 */
void generic_layer_dispose(GenericLayer *self)
{
    if(self->canvas)
        SDL_FreeSurface(self->canvas);
#if USE_SDL_GPU
    if(self->texture)
        GPU_FreeImage(self->texture);
#endif
}

/**
 * @brief Release any resource held by and free
 * the memory used by @p self.
 *
 * @param self a GenericLayer
 */
void generic_layer_free(GenericLayer *self)
{
    generic_layer_dispose(self);
    free(self);
}

/**
 * @brief Loads a file into a newly-created/uninited GenericLayer.
 *
 * Supported formats are those supported by SDL_Image which does the loading.
 * @p self is assumed to be non-inited: No checks are made, no resources
 * are freed.
 *
 * @param self a GenericLayer
 * @param filename The file to read from.
 * @return true on success, false otherwise. The error - as set by SDL_Image -
 * can be retrieved through SDL_GetError.
 */
bool generic_layer_init_from_file(GenericLayer *self, const char *filename)
{
    self->canvas = IMG_Load(filename);
#if USE_SDL_GPU
    self->texture = NULL;
#endif
    return self->canvas != NULL;
}


/**
 * @brief Creates a texture from the canvas.
 *
 * @param self a GenericLayer
 * @return true on success, false otherwise.
 *
 * @note If no texture support is built, this
 * funtion will always return true.
 * TODO: Get rid of the surface ?
 */
bool generic_layer_build_texture(GenericLayer *self)
{
#if USE_SDL_GPU
    self->texture = GPU_CopyImageFromSurface(self->canvas);
    return self->texture != NULL;
#else
    return true;
#endif
}

/**
 * @brief Updates the texture from the content of the canvas.
 *
 * Must be called when you want any modifications done to the
 * surface done after the call to generic_layer_build_texture
 * to be visible by the GPU.
 *
 * @param self a GenericLayer
 *
 * @see generic_layer_build_texture
 */
void generic_layer_update_texture(GenericLayer *self)
{
#if USE_SDL_GPU
    if(self->texture)
        GPU_UpdateImage(self->texture, NULL, self->canvas, NULL);
    else
        generic_layer_build_texture(self);
#endif
}


