/*
 * End to end, one request in and one response out, driven over a socketpair
 * rather than a listening socket. This covers the whole path the server takes
 * for a connection -- parse_request -> global_req_handler -> the cache and the
 * response writer -- with no ports, no threads and no timing involved.
 */
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"

/* Asserts a response carries exactly the bytes of site/<name>. */
static void assert_body_matches_file(captured_response *res, const char *name)
{
    size_t expected_len = 0;
    char *expected = read_site_file(name, &expected_len);
    cr_assert_not_null(expected, "site/%s is missing from the test fixture", name);

    cr_assert_eq(res->body_len, expected_len,
        "served %zu bytes of site/%s, the file holds %zu", res->body_len, name, expected_len);
    cr_assert_arr_eq(res->body, expected, expected_len,
        "the body served does not match site/%s byte for byte", name);

    free(expected);
}

Test(serve, root_serves_index_html)
{
    enter_temp_site();

    captured_response res = do_request("GET / HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(res.status_code, 200, "expected 200 for /, got %d", res.status_code);
    assert_body_matches_file(&res, "index.html");
    free_captured(&res);
}

Test(serve, an_existing_file_is_served_verbatim)
{
    enter_temp_site();
    const char *page = "<h1>about</h1><p>hello</p>";
    write_site_file("about.html", page, strlen(page));

    captured_response res = do_request("GET /about.html HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(res.status_code, 200);
    assert_body_matches_file(&res, "about.html");
    free_captured(&res);
}

Test(serve, an_unknown_path_serves_not_found_with_404)
{
    enter_temp_site();

    captured_response res = do_request("GET /nope.html HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(res.status_code, 404, "expected 404 for a missing page, got %d", res.status_code);
    assert_body_matches_file(&res, "not_found.html");
    free_captured(&res);
}

Test(serve, the_response_declares_content_length_and_close)
{
    enter_temp_site();

    captured_response res = do_request("GET / HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(res.content_length, (long)res.body_len,
        "declared Content-Length %ld but sent %zu bytes", res.content_length, res.body_len);
    cr_assert_not_null(strstr(res.raw, "Connection: close"));
    cr_assert_not_null(strstr(res.raw, "Content-Type: text/html"));
    free_captured(&res);
}

/*
 * The second request for a path is answered from the cache rather than from
 * disk. It has to come back identical.
 */
Test(serve, a_repeated_request_serves_the_same_bytes)
{
    enter_temp_site();
    const char *page = "<h1>about</h1><p>hello</p>";
    write_site_file("about.html", page, strlen(page));

    captured_response first = do_request("GET /about.html HTTP/1.1\r\nHost: x\r\n\r\n");
    captured_response second = do_request("GET /about.html HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(second.status_code, first.status_code);
    cr_assert_eq(second.body_len, first.body_len,
        "first request served %zu bytes, the cached one served %zu", first.body_len, second.body_len);
    cr_assert_arr_eq(second.body, first.body, first.body_len);

    free_captured(&first);
    free_captured(&second);
}

/*
 * FAILS TODAY, on purpose. On a cache hit, send_http_static_page_response()
 * recomputes the body length as strlen(content) instead of keeping the length
 * read_file() measured, and get_cached() hands back a strdup. Both stop at the
 * first NUL, so a file holding one is served whole on the first request and
 * truncated on every request after it.
 *
 * Fixing it means storing the length alongside the body in the cache. Until
 * then this is the test that says so.
 */
Test(serve, a_cached_file_is_not_truncated_at_a_nul_byte)
{
    enter_temp_site();
    const char raw[] = "<h1>AAAA</h1>\0<h1>BBBB</h1>xx"; /* 29 bytes */
    size_t raw_len = sizeof(raw) - 1;
    write_site_file("nul.html", raw, raw_len);

    captured_response first = do_request("GET /nul.html HTTP/1.1\r\nHost: x\r\n\r\n");
    captured_response second = do_request("GET /nul.html HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(first.body_len, raw_len,
        "first request served %zu of %zu bytes", first.body_len, raw_len);
    cr_assert_eq(second.body_len, raw_len,
        "the cached response served %zu of %zu bytes", second.body_len, raw_len);
    cr_assert_arr_eq(second.body, raw, raw_len);

    free_captured(&first);
    free_captured(&second);
}

/*
 * ---------------------------------------------------------------------------
 * The tests below all fail against the current code. Each one describes a bug
 * rather than a deliberate limitation, and is written as the behaviour that
 * should hold, so fixing the bug turns the test green. KNOWN_ISSUES.md has the
 * matching write-up for each.
 * ---------------------------------------------------------------------------
 */

/*
 * FAILS TODAY (crashes). When read_file() cannot load a page,
 * send_http_static_page_response() retries itself with NOT_FOUND_PATH -- but it
 * does that unconditionally, including when the file it failed to read *was*
 * NOT_FOUND_PATH. With site/not_found.html missing or unreadable, one request
 * for any absent page recurses until the stack runs out.
 *
 * That kills the whole process, not just the connection, because a stack
 * overflow on a connection thread takes the server down with it. Verified: the
 * server exits with signal 11 on the first such request.
 *
 * The recursion needs a base case: if the path being loaded is already
 * NOT_FOUND_PATH, send something built in rather than recursing.
 */
Test(serve, a_missing_not_found_page_does_not_take_the_process_down, .timeout = 10)
{
    enter_temp_site();
    remove_site_file("not_found.html");

    captured_response res = do_request("GET /nope.html HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_neq(res.status_code, -1, "the server sent nothing back at all");
    free_captured(&res);
}

/*
 * FAILS TODAY (404). The request path is used to build a filename exactly as it
 * arrived, so the query string becomes part of the path that gets stat'ed:
 * "./site/index.html?v=1" does not exist, and a page that works without a query
 * string 404s with one. Browsers append these constantly for cache busting.
 *
 * The path needs truncating at the first '?' before it is joined to SITE_DIR.
 */
Test(serve, a_query_string_is_ignored_when_resolving_the_path)
{
    enter_temp_site();

    captured_response res = do_request("GET /index.html?v=1 HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(res.status_code, 200,
        "expected 200 for /index.html?v=1, got %d", res.status_code);
    assert_body_matches_file(&res, "index.html");
    free_captured(&res);
}

/*
 * FAILS TODAY (404). Percent-escapes are never decoded, so a file whose name
 * contains a space -- or any character a browser escapes -- can be listed in
 * site/ and still be unreachable. "%20" is looked for literally in the filename.
 *
 * The path needs percent-decoding before it is joined to SITE_DIR.
 */
Test(serve, a_percent_encoded_path_resolves_to_the_real_file)
{
    enter_temp_site();
    const char *page = "<h1>spaced</h1>";
    write_site_file("my page.html", page, strlen(page));

    captured_response res = do_request("GET /my%20page.html HTTP/1.1\r\nHost: x\r\n\r\n");

    cr_assert_eq(res.status_code, 200,
        "expected 200 for /my%%20page.html, got %d", res.status_code);
    assert_body_matches_file(&res, "my page.html");
    free_captured(&res);
}
