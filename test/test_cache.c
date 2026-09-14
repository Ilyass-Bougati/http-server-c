/*
 * The page cache. It is a process-global with no reset, which is exactly why
 * these run under Criterion: every test gets its own process, so each one
 * starts from an empty cache.
 */
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include "cache.h"

/* 29 bytes with a NUL at offset 13, for the cases where the stored length has
 * to be the one that was handed in rather than one recovered from the bytes. */
static const char NUL_BODY[] = "<h1>AAAA</h1>\0<h1>BBBB</h1>xx";
#define NUL_BODY_LEN (sizeof(NUL_BODY) - 1)

/*
 * cache() copies what it is given, so the caller keeps ownership of the buffer
 * it passes and has to free it afterwards. store() does both, so the tests read
 * as one line per entry.
 */
static void store(char *path, const void *data, size_t len)
{
    char *buf = malloc(len + 1);
    memcpy(buf, data, len);
    buf[len] = '\0';
    cache(path, buf, (long)len);
    free(buf);
}

Test(cache, a_path_that_was_never_stored_is_a_miss)
{
    cr_assert_null(get_cached("./site/index.html"));
}

Test(cache, stores_and_returns_content)
{
    char *path = "./site/index.html";
    const char *body = "<h1>index</h1>";
    store(path, body, strlen(body));

    site_page *got = get_cached(path);
    cr_assert_not_null(got);
    cr_assert_str_eq(got->site_content, body);
    cr_assert_eq(got->len, (long)strlen(body));
    free(got);
}

/*
 * The length is whatever the caller measured, not something recovered from the
 * bytes afterwards. This is what keeps a body holding a NUL intact across a
 * cache hit.
 */
Test(cache, keeps_the_length_it_was_given)
{
    char *path = "./site/nul.html";
    store(path, NUL_BODY, NUL_BODY_LEN);

    site_page *got = get_cached(path);
    cr_assert_not_null(got);
    cr_assert_eq(got->len, (long)NUL_BODY_LEN,
        "stored %ld bytes, expected %zu -- the length must not come from strlen",
        got->len, NUL_BODY_LEN);
    cr_assert_arr_eq(got->site_content, NUL_BODY, NUL_BODY_LEN);
    free(got);
}

Test(cache, entries_are_independent)
{
    store("./site/a.html", "aaa", 3);
    store("./site/b.html", "bbbb", 4);

    site_page *a = get_cached("./site/a.html");
    site_page *b = get_cached("./site/b.html");

    cr_assert_str_eq(a->site_content, "aaa");
    cr_assert_eq(a->len, 3);
    cr_assert_str_eq(b->site_content, "bbbb");
    cr_assert_eq(b->len, 4);

    free(a);
    free(b);
}

/*
 * get_cached() hands back a fresh site_page the caller owns and has to free,
 * but the body it points at is the cache's own copy and must not be freed.
 * That split is what send_http_static_page_response() relies on when it calls
 * free(page) and leaves the content alone, so it is worth pinning.
 */
Test(cache, returns_a_fresh_handle_onto_a_shared_body)
{
    char *path = "./site/index.html";
    const char *body = "<h1>index</h1>";
    store(path, body, strlen(body));

    site_page *first = get_cached(path);
    site_page *second = get_cached(path);

    cr_assert_not_null(first);
    cr_assert_not_null(second);
    cr_assert_neq(first, second, "each call must return a separate handle the caller owns");
    cr_assert_eq(first->site_content, second->site_content,
        "the body itself is shared, so freeing it would corrupt the cache");
    cr_assert_eq(first->len, second->len);

    /* Only the handles. Freeing site_content here would be a use-after-free on
     * the next request for this path. */
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
    store(path, "first", 5);
    store(path, "second", 6);

    site_page *got = get_cached(path);
    cr_assert_str_eq(got->site_content, "first");
    cr_assert_eq(got->len, 5);
    free(got);
}
