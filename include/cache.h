#pragma once
#include "../vendor/fnv.h"
#include <stdbool.h>

/*
 * One cached page: the FNV-1a hash of its request path, the file body it maps
 * to, and how many bytes that body is. The path itself is not kept, only its
 * hash, so lookups compare hashes rather than strings -- and two paths whose
 * hashes collide would therefore share an entry.
 *
 * `site_content` is a byte buffer of exactly `len` bytes with no terminating
 * NUL, since a page body may contain NUL bytes of its own.
 */
typedef struct site_page
{
    Fnv32_t hash;
    char *site_content;
    long len;
} site_page;

/*
 * Stores a copy of `content` in the cache under the hash of `path`.
 * path:    the file path used as the cache key; hashed, not copied or kept.
 * content: the page body. It is copied, so the caller keeps ownership and is
 *          the one that frees it. May hold NUL bytes; it is never treated as
 *          a string.
 * len:     how many bytes of `content` to store. This is the length the cache
 *          reports back, so it must be the real byte count and not a strlen.
 * Returns nothing. Does nothing at all if `path` is already cached, so the
 * first body stored for a path is the one that sticks -- which is why editing
 * an already-requested file needs a restart. Aborts the process if the cache
 * cannot grow.
 */
void cache(char *path, char *content, long len);

/*
 * Looks up the entry previously stored for `path`.
 * path: the file path used as the cache key.
 * Returns NULL if the path was never cached. Otherwise a heap-allocated copy
 * of the cache entry, which the caller owns and must free -- but the
 * `site_content` it points at belongs to the cache and must not be freed or
 * modified. Free the struct, leave the body alone.
 *
 * That body is exactly `len` bytes with no terminating NUL, so read it with
 * `len` and never with strlen or any other str* function.
 */
site_page *get_cached(char *path);
