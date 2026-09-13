/*
 * The page cache. It is a process-global with no reset, which is exactly why
 * these run under Criterion: every test gets its own process, so each one
 * starts from an empty cache.
 */
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"

Test(cache, a_path_that_was_never_stored_is_a_miss)
{
    cr_assert_null(get_cached("./site/index.html"));
    cr_assert(!is_cached("./site/index.html"));
}

Test(cache, stores_and_returns_content)
{
    char *path = "./site/index.html";
    cache(path, strdup("<h1>index</h1>"));

    cr_assert(is_cached(path));

    char *got = get_cached(path);
    cr_assert_not_null(got);
    cr_assert_str_eq(got, "<h1>index</h1>");
    free(got);
}

Test(cache, entries_are_independent)
{
    cache("./site/a.html", strdup("aaa"));
    cache("./site/b.html", strdup("bbb"));

    char *a = get_cached("./site/a.html");
    char *b = get_cached("./site/b.html");

    cr_assert_str_eq(a, "aaa");
    cr_assert_str_eq(b, "bbb");
    free(a);
    free(b);
}

/*
 * get_cached() hands back a fresh copy the caller has to free, not the stored
 * pointer. The comment on the prototype in cache.h says the opposite ("owned
 * by the cache: read it, do not free"); the implementation strdup's. This test
 * pins the behaviour the calling code in response.c actually relies on, since
 * that frees what it gets back on a cache hit.
 */
Test(cache, get_cached_returns_a_fresh_copy_each_time)
{
    char *path = "./site/index.html";
    cache(path, strdup("<h1>index</h1>"));

    char *first = get_cached(path);
    char *second = get_cached(path);

    cr_assert_not_null(first);
    cr_assert_not_null(second);
    cr_assert_neq(first, second, "each call must return a separate buffer the caller owns");
    cr_assert_str_eq(first, second);

    free(first);
    free(second);
}

/*
 * Caching the same path twice keeps the first body. That is what makes an
 * edited file need a restart, and it is deliberate.
 */
Test(cache, storing_a_path_twice_keeps_the_first_body)
{
    char *path = "./site/index.html";
    cache(path, strdup("first"));
    cache(path, strdup("second"));

    char *got = get_cached(path);
    cr_assert_str_eq(got, "first");
    free(got);
}
