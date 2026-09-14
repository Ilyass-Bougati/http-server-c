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
            free(content);
            send_http_not_found_page_response(req, res);
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
    if (write(req->client_fd, formatted_header, strlen(formatted_header)) < -1)
    {
        LOG_E("Error writing response header to client fd");
    }

    if (write(req->client_fd, content, out_len) < -1)
    {
        LOG_E("Error writing response content to client fd");
    }

    free(req);
    free(res);
    if (cache_hit)
    {
        free(page);
    }
    else
    {
        free(content);
    }
    free(formatted_header);
}

void send_http_not_found_page_response(http_request *req, http_static_page_response *res)
{
    char *content;
    size_t out_len;
    char *path = NOT_FOUND_PATH;
    site_page *page = get_cached(path);
    bool cache_hit = (page != NULL);
    bool skip_free = false;

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
            free(content);
            skip_free = true;
            content = NOT_FOUND_HTML;
            out_len = strlen(content);
        }
        cache(path, content, out_len);
    }

    basic_headers headers = {
        .content_length = out_len,
        .content_type = "text/html",
        .status_code = 404,
        .status_text = ""};

    char *formatted_header = basic_header_to_string(headers);
    if (write(req->client_fd, formatted_header, strlen(formatted_header)) < -1)
    {
        LOG_E("Error writing response header to client fd");
    }

    if (write(req->client_fd, content, out_len) < -1)
    {
        LOG_E("Error writing response content to client fd");
    }

    free(req);
    free(res);
    if (cache_hit)
    {
        free(page);
    }
    else if (!skip_free)
    {
        free(content);
    }
    free(formatted_header);
}

void send_file_response(http_request *req, http_static_page_response *res)
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
            free(content);
            send_http_not_found_page_response(req, res);
            return;
        }
        cache(path, content, out_len);
    }

    // sending the simple HTTP header
    char *suffix = get_file_suffix(path);
    basic_headers headers = {
        .content_length = out_len,
        .content_type = content_type_from_suffix(suffix),
        .status_code = res->status_code,
        .status_text = ""};

    char *formatted_header = basic_header_to_string(headers);
    if (write(req->client_fd, formatted_header, strlen(formatted_header)) < -1)
    {
        LOG_E("Error writing response header to client fd");
    }

    if (write(req->client_fd, content, out_len) < -1)
    {
        LOG_E("Error writing response content to client fd");
    }

    free(req);
    free(res);
    // free(suffix);
    if (cache_hit)
    {
        free(page);
    }
    else
    {
        free(content);
    }
    free(formatted_header);
}