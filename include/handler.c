#include "handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "request.h"
#include "response.h"
#include <sys/types.h>
#include <sys/stat.h>

void global_req_handler(http_request* req)
{
    char *path;
    http_static_page_response *res;
    int status_code = 200;

    if (strcmp(req->path, "/") == 0) {
        path = SITE_DIR "/index.html";
    } else {
        int path_size = strlen(SITE_DIR) + strlen(req->path) + 1;
        path = calloc(sizeof(char), path_size);
        snprintf(path, path_size, "%s%s", SITE_DIR, req->path);
    }

    struct stat st;
    if (stat(path, &st) < 0 || !S_ISREG(st.st_mode)) {
        printf("DOESN'T EXIST!!!!");
        path = NOT_FOUND_PATH;
        status_code = 404;
    }

    res = create_response(status_code, path);
    send_http_static_page_response(req, res);
}
