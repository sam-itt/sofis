#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include <curl/curl.h>

#include "misc.h"

int http_download_file(char *url, char *output)
{
    CURL *curl;
    FILE *fp;
    CURLcode res;
    bool ret;

    ret = create_path(output);
    if(!ret) return -1;

//  printf("Query: %s\n",url);
    curl = curl_easy_init();
    if(!curl) return -1;

    fp = fopen(output,"wb"); /*TODO: Check*/
    if(!fp){
        printf("Couldn't open %s for writting\n",output);
        return -1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl,
        CURLOPT_USERAGENT, "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_6) "
        "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/69.0.3497.100 Safari/537.36"
    );
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    res = curl_easy_perform(curl);
    /* always cleanup */
    curl_easy_cleanup(curl);
    fclose(fp);
    return 0;
}


