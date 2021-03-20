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
        CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) "
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36"
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
    if(res == CURLE_HTTP_RETURNED_ERROR){
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


