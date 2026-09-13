/* memmem() and mkdtemp() are behind this. */
#define _GNU_SOURCE

#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "http.h"

#define INDEX_HTML "<h1>index</h1>"
#define NOT_FOUND_HTML "<h1>not found</h1>"

void enter_temp_site(void)
{
    static char dir[] = "/tmp/http-server-c-test-XXXXXX";
    if (mkdtemp(dir) == NULL) {
        perror("mkdtemp");
        exit(1);
    }
    if (chdir(dir) != 0) {
        perror("chdir");
        exit(1);
    }
    if (mkdir("site", 0755) != 0) {
        perror("mkdir site");
        exit(1);
    }

    write_site_file("index.html", INDEX_HTML, strlen(INDEX_HTML));
    write_site_file("not_found.html", NOT_FOUND_HTML, strlen(NOT_FOUND_HTML));
}

void write_site_file(const char *name, const void *data, size_t len)
{
    char path[512];
    snprintf(path, sizeof(path), "site/%s", name);

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        perror(path);
        exit(1);
    }
    if (len > 0 && fwrite(data, 1, len, f) != len) {
        perror("fwrite");
        exit(1);
    }
    fclose(f);
}

void remove_site_file(const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "site/%s", name);
    unlink(path);
}

char *read_site_file(const char *name, size_t *out_len)
{
    char path[512];
    snprintf(path, sizeof(path), "site/%s", name);

    struct stat st;
    if (stat(path, &st) != 0) return NULL;

    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;

    char *buf = malloc((size_t)st.st_size + 1);
    size_t n = fread(buf, 1, (size_t)st.st_size, f);
    fclose(f);

    buf[n] = '\0';
    *out_len = n;
    return buf;
}

/* Pulls the status code and Content-Length out of the header block and finds
 * where the body starts. Everything is located by hand rather than with a
 * parser, so the test does not depend on the code it is testing. */
static void parse_captured(captured_response *res)
{
    res->status_code = -1;
    res->content_length = -1;
    res->body = NULL;
    res->body_len = 0;

    if (res->raw_len == 0) return;

    sscanf(res->raw, "HTTP/1.1 %d", &res->status_code);

    const char *cl = strstr(res->raw, "Content-Length: ");
    if (cl != NULL) res->content_length = strtol(cl + strlen("Content-Length: "), NULL, 10);

    /* memmem rather than strstr: the body may contain NUL bytes, and so the
     * response as a whole is not a C string. */
    char *sep = memmem(res->raw, res->raw_len, "\r\n\r\n", 4);
    if (sep != NULL) {
        res->body = sep + 4;
        res->body_len = res->raw_len - (size_t)(res->body - res->raw);
    }
}

captured_response do_request(const char *request)
{
    captured_response res = {0};

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        perror("socketpair");
        exit(1);
    }

    /* Write the request, then close our writing side so the single read() in
     * parse_request() sees the whole thing and does not block. */
    size_t len = strlen(request);
    if (len > 0 && write(sv[0], request, len) != (ssize_t)len) {
        perror("write request");
        exit(1);
    }
    shutdown(sv[0], SHUT_WR);

    parse_request(sv[1]);

    /* parse_request() does not close the socket; handle_request() does that
     * for a real connection. Closing it here is what ends the read loop. */
    close(sv[1]);

    size_t cap = 8192;
    res.raw = malloc(cap);
    for (;;) {
        if (res.raw_len == cap) {
            cap *= 2;
            res.raw = realloc(res.raw, cap);
        }
        ssize_t n = read(sv[0], res.raw + res.raw_len, cap - res.raw_len);
        if (n <= 0) break;
        res.raw_len += (size_t)n;
    }
    close(sv[0]);

    parse_captured(&res);
    return res;
}

void free_captured(captured_response *res)
{
    free(res->raw);
    res->raw = NULL;
    res->body = NULL;
}
