#pragma once

/* Largest request, in bytes, that is read off a connection in one go. */
#define REQUEST_BUFFER_SIZE 8192

/*
 * Reads one request from a connected socket and serves it. Reads up to
 * REQUEST_BUFFER_SIZE bytes, parses the request line into method, path and
 * version, logs it, then hands it to global_req_handler.
 * client_fd: connected client socket, read from and written to.
 * Returns nothing. Only the request line is parsed; headers are ignored.
 */
void parse_request(int client_fd);

/*
 * Thread entry point for one connection, passed to pthread_create.
 * arg: a heap-allocated int holding the client socket descriptor. This call
 *      frees it, so the caller must not.
 * Returns NULL always; the thread's result is the response it wrote. Owns the
 * connection for the thread's lifetime and closes the socket before returning,
 * which is what the "Connection: close" response header promises.
 */
void *handle_request(void *arg);
