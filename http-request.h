#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <stdbool.h>
#include "http-buffer.h"

bool http_request(const char *url, HttpBuffer **buffer);
#endif /* HTTP_REQUEST_H */
