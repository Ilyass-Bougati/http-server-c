#include "cache.h"
#include "../vendor/fnv.h"
#include "log.h"
#include <pthread.h>
#include <stdlib.h>

static site_page **site_cache = NULL;
static int cache_size = 0;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/*
 * Reports whether `path` currently has an entry in the cache.
 * path: the file path used as the cache key.
 * Returns true if a matching entry exists, false otherwise.
 * The difference between this and is_cached(char*), is that
 * this one doesn't lock the cache
 */
static bool is_cached_nl(char *path)
{
    for (int i = 0; i < cache_size; i++)
    {
        if (fnv_equal(path, site_cache[i]->hash))
        {
            return true;
        }
    }

    return false;
}

void cache(char *path, char *content)
{
    pthread_mutex_lock(&m);
    if (is_cached_nl(path)) {
        pthread_mutex_unlock(&m);
        return;
    }

    site_page *page = (site_page*) malloc(sizeof(site_page));
    page->hash = fnv_32a_str(path, FNV1_32A_INIT);
    page->site_content = content;

    site_cache = (site_page**) realloc(site_cache, (cache_size + 1) * sizeof(site_page *));
    if (site_cache == NULL)
    {
        LOG_E("error caching page");
        exit(1);
    }

    site_cache[cache_size++] = page;
    LOG_D("cached %s (hash %ul)", path, (unsigned long) page->hash);
    pthread_mutex_unlock(&m);
}

bool fnv_equal(char *string, Fnv32_t hash)
{
    Fnv32_t h = fnv_32a_str(string, FNV1_32A_INIT);
    return h == hash;
}

bool is_cached(char *path)
{
    pthread_mutex_lock(&m);
    for (int i = 0; i < cache_size; i++)
    {
        if (fnv_equal(path, site_cache[i]->hash))
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
    pthread_mutex_lock(&m);
    for (int i = 0; i < cache_size; i++)
    {
        if (fnv_equal(path, site_cache[i]->hash))
        {
            pthread_mutex_unlock(&m);
            return site_cache[i]->site_content;
        }
    }

    pthread_mutex_unlock(&m);
    return NULL;
}
