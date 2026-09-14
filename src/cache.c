#include "cache.h"
#include "../vendor/fnv.h"
#include "log.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define hash_str(string) fnv_32a_str((string), FNV1_32A_INIT)

static site_page **site_cache = NULL;
static int cache_size = 0;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/*
 * Copies `len` bytes of `content` onto the heap for the cache to keep.
 * content: bytes to copy; may hold NUL bytes, so it is not treated as a string.
 * len:     how many bytes to copy.
 * Returns a heap buffer of exactly `len` bytes, with no terminating NUL. It is
 * a byte buffer and not a C string, so read it with its length and never with
 * strlen, strdup or any other str* function.
 */
static char *copy_content(char *content, int len)
{
    char *copy = (char *)calloc(sizeof(char), len);
    for (int i = 0; i < len; i++)
    {
        copy[i] = content[i];
    }
    return copy;
}

/*
 * Reports whether `hash` currently has an entry in the cache.
 * hash: the hashed file path used as the cache key.
 * Returns true if a matching entry exists, false otherwise.
 * Assumes the caller already holds the cache mutex, which is why it does not
 * take it itself: every caller is already inside the locked section.
 */
static bool is_cached_hash(Fnv32_t hash)
{
    for (int i = 0; i < cache_size; i++)
    {
        if (hash == site_cache[i]->hash)
        {
            return true;
        }
    }

    return false;
}

void cache(char *path, char *content, long len)
{
    Fnv32_t hash = hash_str(path);

    pthread_mutex_lock(&m);
    if (is_cached_hash(hash))
    {
        pthread_mutex_unlock(&m);
        return;
    }

    site_page *page = (site_page *)malloc(sizeof(site_page));
    page->hash = hash;
    page->site_content = copy_content(content, len);
    page->len = len;

    site_cache = (site_page **)realloc(site_cache, (cache_size + 1) * sizeof(site_page *));
    if (site_cache == NULL)
    {
        LOG_E("error caching page");
        exit(1);
    }

    site_cache[cache_size++] = page;
    pthread_mutex_unlock(&m);

    LOG_D("cached %s (hash %lu)", path, (unsigned long)page->hash);
}

site_page *get_cached(char *path)
{
    Fnv32_t hash = hash_str(path);
    pthread_mutex_lock(&m);
    for (int i = 0; i < cache_size; i++)
    {
        if (hash == site_cache[i]->hash)
        {
            site_page *site = calloc(sizeof(site_page), 1);
            memcpy(site, site_cache[i], sizeof(site_page));
            pthread_mutex_unlock(&m);
            return site;
        }
    }

    pthread_mutex_unlock(&m);
    return NULL;
}
