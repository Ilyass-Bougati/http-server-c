#pragma once

/*
 * One incoming request: the socket it arrived on plus the fields taken from
 * its request line. `host` and `agent` are reserved for header parsing and
 * are not populated yet.
 */
typedef struct http_request {
    int client_fd;
    char method[8];
    char path[2048];
    char version[8];
    char* host;
    char* agent;
} http_request;

/*
 * Allocates an empty request.
 * Takes no arguments.
 * Returns a heap-allocated http_request whose fields are uninitialised, so
 * fill every field you read. The caller owns it until it is passed to a
 * handler, which frees it.
 */
http_request* init_request();

/*
 * Writes one INFO log line for a request, in the form "METHOD PATH VERSION".
 * req: the request to log; its request-line fields must already be set.
 * Returns nothing; output goes to stderr through the logger.
 */
void log_http_req(http_request* req);
