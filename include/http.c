#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "request.h"
#include "handler.h"

void parse_request(int client_fd)
{
    char* request_string = (char *) calloc(sizeof(char), REQUEST_BUFFER_SIZE);
    read(client_fd, request_string, REQUEST_BUFFER_SIZE);

    http_request* req = init_request();
    sscanf(request_string, "%s %s %s", req->method, req->path, req->version);
    req->client_fd = client_fd;
    log_http_req(req);
    global_req_handler(req);
    free(request_string);
}
