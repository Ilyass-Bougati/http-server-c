#pragma once
#include <stdio.h>
#include "request.h"

/*
 * Routes a parsed request to a file under SITE_DIR and sends the response.
 * "/" maps to index.html; any other path is appended to SITE_DIR. If the
 * resulting path is not an existing regular file, not_found.html is sent
 * with status 404, otherwise the file is sent with status 200.
 * req: the parsed request; its client_fd is the socket written to. Ownership
 *      passes to this call, which frees it once the response is sent.
 * Returns nothing. The result is the bytes written to the client socket.
 */
void global_req_handler(http_request* req);
