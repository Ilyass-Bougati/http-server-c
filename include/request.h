#pragma once

typedef struct http_request {
    int client_fd;
    char method[8];
    char path[2048];
    char version[8];
    char* host;
    char* agent;
} http_request;

http_request* init_request();

void log_http_req(http_request* req);
