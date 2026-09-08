#include "cache.h"
#include "../vendor/fnv.h"
#include "log.h"
#include <stdlib.h>

__site_page **__site_cache = NULL;
int __cache_size = 0;

void cache(char *path, char *content)
{
    if (is_cached(path)) {
        return;
    }

    __site_page *page = (__site_page*) malloc(sizeof(__site_page));
    page->hash = fnv_32a_str(path, FNV1_32A_INIT);
    page->site_content = content;

    __site_cache = (__site_page**) realloc(__site_cache, __cache_size + 1);
    if (__site_cache == NULL)
    {
        LOG_E("error caching page");
        exit(1);
    }

    __site_cache[__cache_size++] = page;
    LOG_D("cached %s (hash %ul)", path, (unsigned long) page->hash);
}

bool fnv_equal(char *string, Fnv32_t hash)
{
    Fnv32_t h = fnv_32a_str(string, FNV1_32A_INIT);
    return h == hash;
}

bool is_cached(char *path)
{
    for (int i = 0; i < __cache_size; i++)
    {
        if (fnv_equal(path, __site_cache[i]->hash))
        {
            return true;
        }
    }

    return false;
}

char *get_cached(char* path)
{
    for (int i = 0; i < __cache_size; i++)
    {
        if (fnv_equal(path, __site_cache[i]->hash))
        {
            return __site_cache[i]->site_content;
        }
    }

    return NULL;
}
