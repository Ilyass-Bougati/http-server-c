#pragma once
#include <stdio.h>

#define BUFFER_SIZE 1024

void global_req_handler(int client_fd);

long get_file_size(FILE *fptr);
