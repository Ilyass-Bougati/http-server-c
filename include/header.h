#pragma once

/*
 * The minimal set of response header values this server emits: the status
 * line pair (code and reason text) plus Content-Type and Content-Length.
 */
typedef struct http_response_basic_headers {
    int status_code;
    char* status_text;
    char* content_type;
    long content_length;
} basic_headers;

/*
 * Renders a header block into the wire format: status line, Content-Type,
 * Content-Length, "Connection: close", and the blank line ending the block.
 * header: the values to render; all string fields must be NUL-terminated.
 * Returns a heap-allocated NUL-terminated string that the caller must free.
 * Output longer than the internal 1024-byte limit is truncated.
 */
char *basic_header_to_string(basic_headers header);
