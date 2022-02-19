//
//  curl_helper.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include "curl_helper.h"
#include <curl/curl.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct buffer_info {
    size_t size;  /* allocated size */
    size_t len;   /* amount of data */
    char *buffer; /* null terminated */
};

size_t receive_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    int n = size * nmemb;
    struct buffer_info *bufp = (struct buffer_info *)userp;
    if (!bufp)
        return 0;
    if (bufp->len + n + 1 > bufp->size) {
        int newsize = bufp->len + n + 1;
        char *newbuf = (char *)malloc(newsize);
        if (bufp->buffer)
            memmove(newbuf, bufp->buffer, bufp->len);
        memmove(newbuf + bufp->len, buffer, n);
        newbuf[bufp->len + size * nmemb] = 0;
        if (bufp->buffer)
            free(bufp->buffer);
        bufp->buffer = newbuf;
        bufp->size = newsize;
        bufp->len = newsize - 1;
    }
    else {
        memmove(bufp->buffer + bufp->len, buffer, n);
        bufp->len += n;
        if (bufp->size > bufp->len)
            bufp->buffer[bufp->len] = 0;
    }
    return size * nmemb;
}

bool get_file(const std::string url_s, const std::string filename) {
    CURLcode result = CURLE_OK;
    CURLcode curl = curl_global_init(CURL_GLOBAL_NOTHING);
    if (curl)
        return false; // initialisation failed, curl functions cannot be used
    CURL *curl_handle = curl_easy_init();

    struct buffer_info *buf = (struct buffer_info *)malloc(sizeof(struct buffer_info));
    if (!buf) {
        fprintf(stderr, "out of memory allocating buffer\n");
        return false;
    }
    buf->size = 0;
    buf->len = 0;
    buf->buffer = NULL;

    const char *url = url_s.c_str();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    //curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, receive_data);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, buf);

    result = curl_easy_perform(curl_handle);
    if (result != 0)
        std::cerr << "Error %d from curl when retrieving " << url << "\n";
    bool res = true;
    if (buf->buffer) {
        FILE *f = fopen(filename.c_str(), "wb");
        if (f) {
            size_t n = fwrite(buf->buffer, 1, buf->len, f);
            if (n != buf->len) {
                std::cerr << "error: " << n << " bytes written to " << filename << "\n";
                res = false;
            }
            fclose(f);
        }
        free(buf->buffer);
    }
    free(buf);
    curl_easy_cleanup(curl_handle);
    return res;
}
