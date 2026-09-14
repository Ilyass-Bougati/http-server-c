#include "header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// this is arbitrary for now, should be changed later
#define BASIC_HEADER_SIZE 1024

const char *BASIC_HEADER_FORMAT = "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n";

char *basic_header_to_string(basic_headers header)
{
    char *formatted_header = (char *)calloc(sizeof(char), BASIC_HEADER_SIZE);
    snprintf(
        formatted_header,
        BASIC_HEADER_SIZE,
        BASIC_HEADER_FORMAT,
        header.status_code,
        header.status_text,
        header.content_type,
        header.content_length);

    return formatted_header;
}

/*
 * Maps a filename suffix to the Content-Type to send for it.
 * suffix: the suffix including its dot, for example ".css". Matched without
 *         regard to case, so ".PNG" and ".png" are the same. May be NULL.
 * Returns a pointer to a string literal owned by the program: read it, never
 * modify or free it. Anything unrecognised, missing or NULL comes back as
 * application/octet-stream, which tells the browser to download rather than
 * guess at the content.
 */
char *content_type_from_suffix(char *suffix)
{
    static const struct
    {
        const char *suffix;
        char *type;
    } types[] = {
        /* charset belongs on text types only; JSON is defined as UTF-8 already */
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "text/javascript; charset=utf-8"},
        {".mjs", "text/javascript; charset=utf-8"},
        {".txt", "text/plain; charset=utf-8"},
        {".csv", "text/csv; charset=utf-8"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".pdf", "application/pdf"},
        {".wasm", "application/wasm"},
        {".zip", "application/zip"},
        {".svg", "image/svg+xml"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".webp", "image/webp"},
        {".avif", "image/avif"},
        {".ico", "image/vnd.microsoft.icon"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".mp3", "audio/mpeg"},
        {NULL, NULL},
    };

    if (suffix == NULL)
    {
        return "application/octet-stream";
    }

    for (int i = 0; types[i].suffix != NULL; i++)
    {
        if (strcasecmp(suffix, types[i].suffix) == 0)
        {
            return types[i].type;
        }
    }

    return "application/octet-stream";
}