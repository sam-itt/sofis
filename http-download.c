/*
 * SPDX-FileCopyrightText: 2021 Samuel Cuella <samuel.cuella@gmail.com>
 *
 * This file is part of SoFIS - an open source EFIS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include <curl/curl.h>

#include "misc.h"

bool http_download_file(char *url, char *output)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    bool ret;

    ret = create_path(output);
    if(!ret) return false;

//  printf("Query: %s\n",url);
    curl = curl_easy_init();
    if(!curl) return false;

    fp = fopen(output,"wb");
    if(!fp){
        printf("Couldn't open %s for writting\n",output);
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl,
        CURLOPT_USERAGENT, "curl/7.68.0"
    );
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    printf("Downloading %s, please wait\t", url);
    fflush(stdout);
    res = curl_easy_perform(curl);
    /* always cleanup */
    curl_easy_cleanup(curl);
    fclose(fp);
    if(res != CURLE_OK){
        unlink(output);
        printf("[ %sFAILED%s ]\n",
            "\033[0;31m", /*red*/
            "\033[0m" /*Reset*/
        );
        return false;
    }

    printf("[ %sOK%s ]\n",
        "\033[0;32m", /*red*/
        "\033[0m" /*Reset*/
    );
    return true;
}


