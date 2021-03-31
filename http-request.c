/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <curl/curl.h>

#include "http-buffer.h"


static size_t handle_response(void *contents, size_t size,
                              size_t nmemb, HttpBuffer *buffer)
{
    bool rv;
    size_t len;

    len = size * nmemb;
    rv = http_buffer_add_content(buffer, contents, len);

    return rv ? len : 0;
}

bool http_request(const char *url, HttpBuffer **buffer)
{
    CURL *curl;
    CURLcode res;
    bool ret;

    *buffer = (*buffer) ? (*buffer) : http_buffer_new(0);
    if(!(*buffer)) return false;

//  printf("Query: %s\n",url);
    curl = curl_easy_init();
    if(!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl,
        CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) "
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36"
    );
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)(*buffer));
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    res = curl_easy_perform(curl);
    /* always cleanup */
    curl_easy_cleanup(curl);
    if(res != CURLE_OK){
        return false;
    }

    return true;
}


