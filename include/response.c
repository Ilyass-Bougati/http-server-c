#include "response.h"
#include "stdlib.h"
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
    // reading the index.html file
    FILE *fptr;
    char *path = res->path;
    fptr = fopen(path, "r");

    if (fptr == NULL) {
        fprintf(stderr, "Error opening %s\n", path);
    }

    // getting the size of the file
    long size = get_file_size(fptr);

    // sending the simple HTTP header
    basic_headers headers = {
        .content_length = size,
        .content_type = "text/html",
        .status_code = 200,
        .status_text = "OK"
    };

    char* formatted_header = basic_header_to_string(headers);
    write(req->client_fd, formatted_header, strlen(formatted_header));

    char *page_line = (char *) calloc(sizeof(char), BUFFER_SIZE);
    while(fgets(page_line, BUFFER_SIZE, fptr)) {
        write(req->client_fd, page_line, strlen(page_line));
    }

    free(req);
    free(res);
    free(page_line);
    free(formatted_header);
}
