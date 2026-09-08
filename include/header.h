#pragma once

typedef struct http_response_basic_headers {
    int status_code;
    char* status_text;
    char* content_type;
    long content_length;
} basic_headers;

char *basic_header_to_string(basic_headers header);
