#pragma once
#include "request.h"
#include <stdio.h>

#define BUFFER_SIZE 1024
#define SITE_DIR "./site"
#define NOT_FOUND_PATH SITE_DIR "/not_found.html"

/*
 * A response that serves one static file: the status code to report and the
 * path of the file whose contents form the body.
 */
typedef struct http_static_page_response {
    int status_code;
    char* path;
} http_static_page_response;

/*
 * Allocates an empty response.
 * Takes no arguments.
 * Returns a heap-allocated, zero-filled http_static_page_response owned by
 * the caller.
 */
http_static_page_response *init_response();

/*
 * Allocates a response and fills it in.
 * status_code: HTTP status to report, for example 200 or 404.
 * path:        path of the file to serve. Stored by pointer, not copied, so
 *              it must stay valid until the response is sent.
 * Returns a heap-allocated response owned by the caller.
 */
http_static_page_response *create_response(int status_code, char *path);

/*
 * Releases a response allocated by init_response or create_response.
 * res: the response to free; its `path` is not freed, since the response does
 *      not own it.
 * Returns nothing. Declared for callers that build a response and abandon it;
 * no implementation is compiled in yet, so linking a call to it will fail.
 */
void free_response(http_static_page_response *res);

/*
 * Sends a static page: loads the file at res->path (serving it from the page
 * cache when possible, otherwise reading it from disk and caching it), then
 * writes the header block followed by the body to the client socket.
 * req: the request being answered; supplies the socket to write to.
 * res: the status code and file path to send.
 * Returns nothing, and frees both `req` and `res` before returning, so
 * neither may be used afterwards. The socket is left open for the caller
 * to close.
 */
void send_http_static_page_response(http_request *req, http_static_page_response *res);
