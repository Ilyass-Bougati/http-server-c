# Known issues

Two lists. **Bugs to fix** are defects — the code does something it did not mean
to, and each one is reproducible. **Accepted limits** are what the server
deliberately does not do; they are the cost of keeping it small, and they are not
on anybody's list to change.

Everything here was reproduced against the current code.

---

## Bugs to fix

Most of these have a test in `test/` that fails today and goes green when the bug
is fixed. Where there is no test, the reason is given — usually that the symptom is
a leak, which has no effect on the response and only shows up when the suite is
built with `-DSANITIZE=address`.

> Fixed already: cached bodies used to be cut at the first NUL byte, because the
> length was recovered with `strlen()` on a cache hit. The cache now carries the
> length it was given. Guarded by `cache::keeps_the_length_it_was_given` and
> `serve::a_cached_file_is_not_truncated_at_a_nul_byte`.

### 1. A missing `not_found.html` takes the whole process down

> failing test: `serve::a_missing_not_found_page_does_not_take_the_process_down`

When `read_file()` cannot load a page, [`send_http_static_page_response()`](src/response.c:47)
retries itself with `NOT_FOUND_PATH`. It does that unconditionally — including
when the file it just failed to read *was* `NOT_FOUND_PATH`. So if
`site/not_found.html` is missing, unreadable, or deleted while the server is up,
one request for any absent page recurses until the stack runs out.

It is not one bad connection: a stack overflow on a connection thread kills the
process. Verified — the server exits with signal 11 on the first such request,
and every open connection dies with it. Under `-DSANITIZE=address` the suite names
it exactly: `stack-overflow ... in read_file`. This is the most serious thing here.

The recursion needs a base case: when the path being loaded is already
`NOT_FOUND_PATH`, send something built into the binary instead of recursing.

### 2. Query strings are not stripped from the path

> failing test: `serve::a_query_string_is_ignored_when_resolving_the_path`

The request path is turned into a filename exactly as it arrived, so the query
string becomes part of it: `stat("./site/index.html?v=1")` fails and a page that
works without a query string 404s with one. Browsers append these constantly for
cache busting, so this is reachable by accident rather than on purpose.

Truncate the path at the first `?` in [`global_req_handler()`](src/handler.c:33)
before joining it to `SITE_DIR`.

### 3. Percent-escapes are never decoded

> failing test: `serve::a_percent_encoded_path_resolves_to_the_real_file`

`%20` and friends are looked for literally in the filename, so a file whose name
contains a space can sit in `site/` and be unreachable: the browser sends
`/my%20page.html`, and the server looks for a file actually named `my%20page.html`.

Percent-decode the path before joining it to `SITE_DIR`. Worth doing together
with #2, in the same place.

### 4. Every 404 leaks its path buffer

> no test — a leak has no effect on the response, so it takes `-DSANITIZE=address`
> to see. It is already reproduced there: the 404 cases in `test_serve` report
> around 8 KB leaked across two allocations.

[`global_req_handler()`](src/handler.c:41) overwrites the `calloc`ed `path` with
`NOT_FOUND_PATH` and never frees it — the `free(path)` on the line above is
commented out. Every 404 leaks the buffer, which on a server with no cache
eviction means it grows for as long as the process lives.

### 5. Caching a path twice leaks the body that loses

> no test — same reason as #4. Reproduced by
> `cache::storing_a_path_twice_keeps_the_first_body` under ASan, which reports the
> 7 bytes the second `cache()` call was handed and never freed.

`cache.h` says the cache takes ownership of `content` and the caller must not
free it. When the path is already cached, [`cache()`](src/cache.c:41) returns
early and drops the pointer without freeing it, so nobody frees it.

Single-threaded this only fires if something stores the same path twice, because a
hit is served from the cache. It fires on its own under concurrency: two
connections that both miss the same path both read the file and both call
`cache()`, and the loser's buffer leaks.

Either free the content on the early return, or document that the caller keeps
ownership when the path was already present.

### 6. `read()` and `write()` return values are ignored

> no test — needs a client that stops reading mid-response, which is fiddly to
> make deterministic

Neither `write()` in [`response.c:62`](src/response.c:62) is checked, so a short
write silently truncates the body and a failed write is indistinguishable from a
successful one. Same for the `read()` in [`http.c:25`](src/http.c:25), where a
`-1` is treated as an empty request. Blocking sockets make short writes rare
rather than impossible, which is what makes this the kind of bug that shows up
once under load and never reproduces.

---

## Accepted limits

These are deliberate. The server is a development server; it is not trying to be
nginx, and none of the below is worth the code it would cost.

### Security

* Nothing keeps the served path inside `site/`. `global_req_handler()`
  concatenates the request path onto `SITE_DIR` and calls `stat`, so
  `GET /../server.c` returns the source file, and every regular file the process
  can read is reachable the same way.
* Nothing resists a client trying to be expensive: no request rate limit, no
  timeouts, no cap on threads.

**Do not expose this to a network you do not control.**

### Concurrency

* Reads and writes have no timeout, so a client that connects and never sends
  holds a thread until it goes away on its own.
* Every connection gets a detached thread with no ceiling, so what limits load
  is thread creation rather than anything the server decides.

### Protocol and caching

* Requests over 8192 bytes (`REQUEST_BUFFER_SIZE`) are truncated, only the
  request line is parsed, and one `read()` per connection means a request line
  split across TCP segments is never reassembled.
* The method is parsed and then ignored: `POST /index.html` gets the same 200
  and the same body as `GET`.
* Every response is labelled `text/html` whatever the file holds, the reason
  phrase is always empty, and `Connection: close` is the only mode.
* Cache entries are keyed by a 32-bit FNV-1a hash and the path is never stored,
  so lookups compare hashes and a collision serves the wrong body. Handling
  collisions would mean a bucket list per entry, which is not worth it here.
* Cache entries are never invalidated or freed, which is why editing an
  already-requested file needs a restart.
