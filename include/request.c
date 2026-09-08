#include "request.h"
#include <stdlib.h>
#include "log.h"

http_request* init_request()
{
    http_request* request = (http_request*) malloc(sizeof(http_request));
    return request;
}

void log_http_req(http_request* req)
{
    LOG_I("%s %s %s", req->method, req->path, req->version);
}
