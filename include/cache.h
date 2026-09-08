#pragma once
#include "../vendor/fnv.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct __site_page {
    Fnv32_t hash;
    char* site_content;
} __site_page;


void cache(char *path, char *content);
char *get_cached(char* path);
bool fnv_equal(char *string, Fnv32_t hash);
bool is_cached(char *path);
