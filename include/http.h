#pragma once

#define REQUEST_BUFFER_SIZE 8192

void parse_request(int client_fd);

void *handle_request(void *arg);
