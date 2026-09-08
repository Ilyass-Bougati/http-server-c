#include "response.h"
#include "cache.h"
#include "stdlib.h"
#include <stdbool.h>
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
    bool cached = is_cached(path);
    char *content;
    size_t out_len;
    if (cached) {
        content = get_cached(path);
        out_len = strlen(content);
    } else {
        content = read_file(path, &out_len);
        cache(path, content);
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
    free(formatted_header);
}
