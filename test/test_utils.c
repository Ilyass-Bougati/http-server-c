/*
 * read_file() and basic_header_to_string(): the two pieces with no sockets and
 * no shared state behind them. These pin the exact bytes that go on the wire,
 * so a change to the header format shows up here rather than in a load test.
 */
#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include "helpers.h"
#include "utils.h"
#include "header.h"

Test(read_file, reads_a_whole_file)
{
    enter_temp_site();
    const char *content = "<h1>page</h1>";
    write_site_file("page.html", content, strlen(content));

    size_t len = 0;
    char *buf = read_file("site/page.html", &len);

    cr_assert_not_null(buf, "read_file returned NULL for a file that exists");
    cr_assert_eq(len, strlen(content), "expected %zu bytes, got %zu", strlen(content), len);
    cr_assert_arr_eq(buf, content, len);
    free(buf);
}

Test(read_file, reports_the_true_length_of_a_file_holding_a_nul)
{
    enter_temp_site();
    /* 29 bytes, with a NUL in the middle. */
    const char raw[] = "<h1>AAAA</h1>\0<h1>BBBB</h1>xx";
    size_t raw_len = sizeof(raw) - 1;
    write_site_file("nul.html", raw, raw_len);

    size_t len = 0;
    char *buf = read_file("site/nul.html", &len);

    cr_assert_not_null(buf);
    cr_assert_eq(len, raw_len, "read_file must report the file size, not strlen");
    cr_assert_arr_eq(buf, raw, raw_len);
    free(buf);
}

Test(read_file, returns_null_for_a_missing_file)
{
    enter_temp_site();
    size_t len = 0;
    cr_assert_null(read_file("site/nope.html", &len));
}

Test(read_file, returns_null_for_a_directory)
{
    enter_temp_site();
    size_t len = 0;
    cr_assert_null(read_file("site", &len), "a directory is not a regular file");
}

Test(header, renders_a_200_block)
{
    basic_headers h = {
        .status_code = 200, .status_text = "", .content_type = "text/html", .content_length = 13
    };

    char *out = basic_header_to_string(h);
    cr_assert_str_eq(out,
        "HTTP/1.1 200 \r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 13\r\n"
        "Connection: close\r\n"
        "\r\n");
    free(out);
}

Test(header, renders_a_404_block)
{
    basic_headers h = {
        .status_code = 404, .status_text = "", .content_type = "text/html", .content_length = 18
    };

    char *out = basic_header_to_string(h);
    cr_assert_str_eq(out,
        "HTTP/1.1 404 \r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 18\r\n"
        "Connection: close\r\n"
        "\r\n");
    free(out);
}

Test(header, ends_with_a_blank_line)
{
    basic_headers h = {
        .status_code = 200, .status_text = "", .content_type = "text/html", .content_length = 0
    };

    char *out = basic_header_to_string(h);
    size_t len = strlen(out);
    cr_assert_geq(len, 4u);
    cr_assert_arr_eq(out + len - 4, "\r\n\r\n", 4, "the header block must be terminated by a blank line");
    free(out);
}
