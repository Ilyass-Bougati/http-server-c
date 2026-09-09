#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include "include/http.h"
#include "include/log.h"

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);

    // assuring the user providede a port
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    // initializing the variables
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    // creating the socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // binding the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // listening
    if (listen(server_fd, 512) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n\n", port);

    while (1) {
        // accepting a connection
        client_fd = accept(server_fd, (struct sockaddr *)&address, &addr_len);
        if (client_fd < 0) {
            LOG_E("error accepting connection");
            continue;
        }
        int *pfd = malloc(sizeof *pfd);
        *pfd = client_fd;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_request, pfd);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
