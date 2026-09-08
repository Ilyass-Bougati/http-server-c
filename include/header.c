#include "header.h"
#include <stdio.h>
#include <stdlib.h>

// this is arbitrary for now, should be changed later
#define BASIC_HEADER_SIZE 1024

const char *BASIC_HEADER_FORMAT = "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n";

char *basic_header_to_string(basic_headers header)
{
    char *formatted_header = (char *) calloc(sizeof(char), BASIC_HEADER_SIZE);
    snprintf(
        formatted_header,
        BASIC_HEADER_SIZE,
        BASIC_HEADER_FORMAT,
        header.status_code,
        header.status_text,
        header.content_type,
        header.content_length
    );

    return formatted_header;
}
