//#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "http-buffer.h"

HttpBuffer *http_buffer_new(size_t len)
{
    HttpBuffer *self;

    self = calloc(1, sizeof(HttpBuffer));
    if(self){
        if(len){
            self->buffer = malloc(sizeof(char) * len);
            if(!self->buffer){
                free(self);
                return(NULL);
            }
            self->allocated = len;
        }
    }
    return(self);
}

/**
 * @brief Ensure that @param self have enough rooom to store @param size bytes
 *
 *
 */
bool http_buffer_resize(HttpBuffer *self, size_t size)
{
    if(self->allocated < size){
        void *tmp = realloc(self->buffer, size);
        if(!tmp) return false;
        self->buffer = tmp;
        self->allocated = size;
    }
    return true;
}

/**
 * @brief Puts @param content into @param self, ensuring @param self is
 * NULL-terminated
 *
 */
bool http_buffer_set_content(HttpBuffer *self, const void *content, size_t len)
{
    bool rv;

    rv = http_buffer_resize(self, len+1); /*TODO: Handle overflow*/
    if(!rv) return 0;

    memcpy(self->buffer, content, len);
    self->buffer[len+1] = '\0';
    self->len = len;

    return true;
}

/**
 * @brief Puts @param content into @param self, ensuring @param self is
 * NULL-terminated
 *
 */
bool http_buffer_add_content(HttpBuffer *self, const void *content, size_t len)
{
    bool rv;

    rv = http_buffer_resize(self, self->allocated+len+1); /*TODO: Handle overflow*/
    if(!rv) return 0;

    memcpy(self->buffer+self->len, content, len);
    self->len += len;
    self->buffer[self->len+1] = '\0';

    return true;
}
