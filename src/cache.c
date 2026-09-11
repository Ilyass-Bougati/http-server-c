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
 * Reports whether `hash` currently has an entry in the cache.
 * hash: the hashed file path used as the cache key.
 * Returns true if a matching entry exists, false otherwise.
 * The difference between this and is_cached(char*), is that
 * this one doesn't lock the cache
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

void cache(char *path, char *content)
{
    Fnv32_t hash = hash_str(path);

    pthread_mutex_lock(&m);
    if (is_cached_hash(hash)) {
        pthread_mutex_unlock(&m);
        return;
    }

    site_page *page = (site_page*) malloc(sizeof(site_page));
    page->hash = hash;
    page->site_content = content;

    site_cache = (site_page**) realloc(site_cache, (cache_size + 1) * sizeof(site_page *));
    if (site_cache == NULL)
    {
        LOG_E("error caching page");
        exit(1);
    }

    site_cache[cache_size++] = page;
    pthread_mutex_unlock(&m);

    LOG_D("cached %s (hash %ul)", path, (unsigned long) page->hash);
}

bool is_cached(char *path)
{
    Fnv32_t hash = hash_str(path);
    pthread_mutex_lock(&m);
    for (int i = 0; i < cache_size; i++)
    {
        if (hash == site_cache[i]->hash)
        {
            pthread_mutex_unlock(&m);
            return true;
        }
    }

    pthread_mutex_unlock(&m);
    return false;
}

char *get_cached(char* path)
{
    Fnv32_t hash = hash_str(path);
    pthread_mutex_lock(&m);
    for (int i = 0; i < cache_size; i++)
    {
        if (hash == site_cache[i]->hash)
        {
            char *content = strdup(site_cache[i]->site_content);
            pthread_mutex_unlock(&m);
            return content;
        }
    }

    pthread_mutex_unlock(&m);
    return NULL;
}
