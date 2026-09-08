#pragma once
#include "request.h"
#include <stdio.h>

#define BUFFER_SIZE 1024
#define SITE_DIR "./site"
#define NOT_FOUND_PATH SITE_DIR "/not_found.html"

typedef struct http_static_page_response {
    int status_code;
    char* path;
} http_static_page_response;

http_static_page_response *init_response();

http_static_page_response *create_response(int status_code, char *path);

void free_response(http_static_page_response *res);

void send_http_static_page_response(http_request *req, http_static_page_response *res);
