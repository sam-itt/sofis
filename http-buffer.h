#ifndef HTTP_BUFFER_H
#define HTTP_BUFFER_H
#include <stdio.h>
#include <stdbool.h>

typedef struct{
    char *buffer;
    size_t allocated;
    size_t len;
}HttpBuffer;

HttpBuffer *http_buffer_new(size_t len);
bool http_buffer_resize(HttpBuffer *self, size_t size);

bool http_buffer_set_content(HttpBuffer *self, const void *content, size_t len);
bool http_buffer_add_content(HttpBuffer *self, const void *content, size_t len);
#endif /* HTTP_BUFFER_H */
