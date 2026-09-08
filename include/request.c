#include "request.h"
#include <stdio.h>
#include <stdlib.h>

http_request* init_request()
{
    http_request* request = (http_request*) malloc(sizeof(http_request));
    return request;
}

void log_http_req(http_request* req)
{
    printf("%s %s %s\n", req->method, req->path, req->version);
}
