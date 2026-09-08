#include "handler.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "header.h"

void global_req_handler(int client_fd)
{
    // reading the index.html file
    FILE *fptr;
    fptr = fopen("index.html", "r");
    if (fptr == NULL) {
        perror("Error opening index.html");
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
    write(client_fd, formatted_header, strlen(formatted_header));

    char *page_line = (char *) calloc(sizeof(char), BUFFER_SIZE);
    while(fgets(page_line, BUFFER_SIZE, fptr)) {
        write(client_fd, page_line, strlen(page_line));
    }
}

long get_file_size(FILE *fptr) {
    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    rewind(fptr);
    return size;
}
