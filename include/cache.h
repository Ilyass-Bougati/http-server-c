#pragma once
#include "../vendor/fnv.h"
#include <stdlib.h>
#include <stdbool.h>

/*
 * One cached page: the FNV-1a hash of its request path and the file body it
 * maps to. The path itself is not kept, only its hash, so lookups compare
 * hashes rather than strings.
 */
typedef struct __site_page {
    Fnv32_t hash;
    char* site_content;
} __site_page;


/*
 * Stores `content` in the cache under the hash of `path`.
 * path:    the file path used as the cache key; hashed, not copied or kept.
 * content: NUL-terminated page body. The cache takes ownership of this
 *          pointer and never frees it, so do not free it after the call.
 * Returns nothing. Does nothing if `path` is already cached. Aborts the
 * process if the cache cannot grow.
 */
void cache(char *path, char *content);

/*
 * Looks up the body previously stored for `path`.
 * path: the file path used as the cache key.
 * Returns the cached buffer, or NULL if the path was never cached. The buffer
 * is owned by the cache: read it, do not free or modify it.
 */
char *get_cached(char* path);

/*
 * Tests whether a string hashes to a given FNV-1a value.
 * string: NUL-terminated string to hash.
 * hash:   the 32-bit FNV-1a value to compare against.
 * Returns true when they match. Note this is a hash comparison, so a
 * collision reports true for two different strings.
 */
bool fnv_equal(char *string, Fnv32_t hash);

/*
 * Reports whether `path` currently has an entry in the cache.
 * path: the file path used as the cache key.
 * Returns true if a matching entry exists, false otherwise.
 */
bool is_cached(char *path);
