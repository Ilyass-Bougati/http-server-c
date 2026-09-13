#include "response.h"
#include "cache.h"
#include "stdlib.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "header.h"
#include "utils.h"
#include <unistd.h>

http_static_page_response *init_response()
{
    http_static_page_response* res = (http_static_page_response*) calloc(sizeof(http_static_page_response), 1);
    return res;
}

http_static_page_response *create_response(int status_code, char* path)
{
    http_static_page_response* res = init_response();
    res->path = path;
    res->status_code = status_code;
    return res;
}

void send_http_static_page_response(http_request *req, http_static_page_response *res)
{
    char *path = res->path;
    char *content = get_cached(path);
    bool cache_hit = (content != NULL);
    size_t out_len;
    if (!cache_hit) {
        content = read_file(path, &out_len);
        if (content == NULL)
        {
            res->path = NOT_FOUND_PATH;
            res->status_code = 404;
            send_http_static_page_response(req, res);
            return;
        }
        cache(path, content);
    } else {
        out_len = strlen(content);
    }

    // sending the simple HTTP header
    basic_headers headers = {
        .content_length = out_len,
        .content_type = "text/html",
        .status_code = res->status_code,
        .status_text = ""
    };

    char* formatted_header = basic_header_to_string(headers);
    write(req->client_fd, formatted_header, strlen(formatted_header));

    write(req->client_fd, content, out_len);

    free(req);
    free(res);
    if (cache_hit)
    {
        free(content);
    }
    free(formatted_header);
}
