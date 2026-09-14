#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "request.h"
#include "handler.h"
#include "log.h"
#include <pthread.h>
#include <syscall.h>
#include <string.h>

void *handle_request(void *arg)
{
    LOG_D("thread start, tid %d", (int)syscall(SYS_gettid));

    int client_fd = *(int *)arg;
    parse_request(client_fd);
    free(arg);
    close(client_fd);
    return NULL;
}

void parse_request(int client_fd)
{
    char *request_string = (char *)calloc(sizeof(char), REQUEST_BUFFER_SIZE);
    if (read(client_fd, request_string, REQUEST_BUFFER_SIZE) == -1)
    {
        perror("Error reading HTTP request");
        exit(1);
    }

    http_request *req = init_request();
    char *path = (char *)calloc(sizeof(char), 2048);
    sscanf(request_string, "%7s %2047s %7s", req->method, path, req->version);
    req->client_fd = client_fd;

    // removing any path variables
    size_t buff_size = 1;
    char *buff = malloc(buff_size);
    if (buff == NULL)
    {
        free(path);
        LOG_E("Couldn't allocate buffer");
        return;
    }
    buff[0] = '\0';

    char *myPtr = strtok(path, "/");
    while (myPtr != NULL)
    {
        if (strcmp(myPtr, "..") != 0)
        {
            buff_size += strlen(myPtr) + 1;
            char *tmp = realloc(buff, buff_size);
            if (tmp == NULL)
            {
                free(buff);
                free(path);
                LOG_E("Couldn't reallocating buffer");
                return;
            }
            buff = tmp;

            strcat(buff, "/");
            strcat(buff, myPtr);
        }
        myPtr = strtok(NULL, "/");
    }

    printf("%s\n", buff);

    for (size_t i = 0; i < strlen(buff); i++)
    {
        if (buff[i] == '\0' || buff[i] == '?')
        {
            req->path[i] = '\0';
            break;
        }
        req->path[i] = buff[i];
    }

    free(path);
    free(buff);
    log_http_req(req);
    global_req_handler(req);
    free(request_string);
}
