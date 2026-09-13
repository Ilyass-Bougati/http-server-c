#include "response.h"
#include "cache.h"
#include "stdlib.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "header.h"
#include "utils.h"
#include <unistd.h>
#include "log.h"

http_static_page_response *init_response()
{
    http_static_page_response *res = (http_static_page_response *)calloc(sizeof(http_static_page_response), 1);
    return res;
}

http_static_page_response *create_response(int status_code, char *path)
{
    http_static_page_response *res = init_response();
    res->path = path;
    res->status_code = status_code;
    return res;
}

void send_http_static_page_response(http_request *req, http_static_page_response *res)
{
    char *content;
    size_t out_len;
    char *path = res->path;
    site_page *page = get_cached(path);
    bool cache_hit = (page != NULL);

    if (cache_hit)
    {
        content = page->site_content;
        out_len = page->len;
        LOG_D("Cache hit (len: %lu)", out_len);
    }
    else
    {
        content = read_file(path, &out_len);
        if (content == NULL)
        {
            res->path = NOT_FOUND_PATH;
            res->status_code = 404;
            send_http_static_page_response(req, res);
            return;
        }
        cache(path, content, out_len);
    }

    // sending the simple HTTP header
    basic_headers headers = {
        .content_length = out_len,
        .content_type = "text/html",
        .status_code = res->status_code,
        .status_text = ""};

    char *formatted_header = basic_header_to_string(headers);
    write(req->client_fd, formatted_header, strlen(formatted_header));

    write(req->client_fd, content, out_len);

    free(req);
    free(res);
    if (cache_hit)
    {
        free(page);
    }
    free(formatted_header);
}
