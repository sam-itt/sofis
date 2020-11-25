#ifndef GENERIC_LAYER_H
#define GENERIC_LAYER_H

#include <SDL2/SDL.h>

#if USE_SDL_GPU
#include <SDL_gpu.h>
#endif

typedef struct{
    SDL_Surface *canvas;
#if USE_SDL_GPU
    GPU_Image *texture;
#endif
}GenericLayer;

#define GENERIC_LAYER(self) ((GenericLayer *)(self))
#define generic_layer_lock(self) SDL_LockSurface((self)->canvas)
#define generic_layer_unlock(self) SDL_UnlockSurface((self)->canvas)

#define generic_layer_w(self) ((self)->canvas->w)
#define generic_layer_h(self) ((self)->canvas->h)

GenericLayer *generic_layer_new(int width, int height);
GenericLayer *generic_layer_new_from_file(const char *filename);

bool generic_layer_init(GenericLayer *self, int width, int height);
bool generic_layer_init_with_masks(GenericLayer *self, int width, int height, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
bool generic_layer_init_from_file(GenericLayer *self, const char *filename);

void generic_layer_dispose(GenericLayer *self);
void generic_layer_free(GenericLayer *self);

bool generic_layer_build_texture(GenericLayer *self);
#endif /* GENERIC_LAYER_H */
