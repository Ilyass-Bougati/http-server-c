# Known issues

Two lists. **Bugs to fix** are defects — the code does something it did not mean
to, and each one is reproducible. **Accepted limits** are what the server
deliberately does not do; they are the cost of keeping it small, and they are not
on anybody's list to change.

Everything here was reproduced against the current code.

---

## Bugs to fix

Most of these have a test in `test/` that fails today and goes green when the bug
is fixed. Where there is no test, the reason is given.

### 1. A missing `not_found.html` takes the whole process down

> failing test: `serve::a_missing_not_found_page_does_not_take_the_process_down`

When `read_file()` cannot load a page, [`send_http_static_page_response()`](src/response.c:37)
retries itself with `NOT_FOUND_PATH`. It does that unconditionally — including
when the file it just failed to read *was* `NOT_FOUND_PATH`. So if
`site/not_found.html` is missing, unreadable, or deleted while the server is up,
one request for any absent page recurses until the stack runs out.

It is not one bad connection: a stack overflow on a connection thread kills the
process. Verified — the server exits with signal 11 on the first such request,
and every open connection dies with it. This is the most serious thing here.

The recursion needs a base case: when the path being loaded is already
`NOT_FOUND_PATH`, send something built into the binary instead of recursing.

### 2. Cached bodies are cut at the first NUL byte

> failing test: `serve::a_cached_file_is_not_truncated_at_a_nul_byte`

On a cache miss the body length is the one `read_file()` measured. On a hit,
[`response.c:42`](src/response.c:42) recomputes it as `strlen(content)`, and
`get_cached()` hands back a `strdup`. Both stop at the first NUL, so a file
holding one is served whole to the first visitor and truncated to everyone after:

```
bin.html on disk: 29 bytes, NUL at offset 13
request 1 (miss)  Content-Length: 29, 29 bytes sent
request 2 (hit)   Content-Length: 13, 13 bytes sent
```

Note the response stays self-consistent — the declared length matches what is
sent — so the k6 `length matches body` check passes while the body is wrong. Only
a test that compares against the file on disk catches it.

The cache has to keep the length alongside the body instead of recovering it
from the bytes.

### 3. Query strings are not stripped from the path

> failing test: `serve::a_query_string_is_ignored_when_resolving_the_path`

The request path is turned into a filename exactly as it arrived, so the query
string becomes part of it: `stat("./site/index.html?v=1")` fails and a page that
works without a query string 404s with one. Browsers append these constantly for
cache busting, so this is reachable by accident rather than on purpose.

Truncate the path at the first `?` in [`global_req_handler()`](src/handler.c:29)
before joining it to `SITE_DIR`.

### 4. Percent-escapes are never decoded

> failing test: `serve::a_percent_encoded_path_resolves_to_the_real_file`

`%20` and friends are looked for literally in the filename, so a file whose name
contains a space can sit in `site/` and be unreachable: the browser sends
`/my%20page.html`, and the server looks for a file actually named `my%20page.html`.

Percent-decode the path before joining it to `SITE_DIR`. Worth doing together
with #3, in the same place.

### 5. Every 404 leaks its path buffer

> no test — a leak has no observable effect on the response, so catching it means
> running the suite under `-DSANITIZE=address`, which reports it as a leak

[`global_req_handler()`](src/handler.c:37) overwrites the `calloc`ed `path` with
`NOT_FOUND_PATH` and never frees it — the `free(path)` on the line above is
commented out. Every 404 leaks the buffer, which on a server with no cache
eviction means it grows for as long as the process lives.

### 6. Caching a path twice leaks the body that loses

> no test — same reason as #5

`cache.h` says the cache takes ownership of `content` and the caller must not
free it. When the path is already cached, [`cache()`](src/cache.c:41) returns
early and drops the pointer without freeing it, so nobody frees it.

Single-threaded this never fires, because a hit is served from the cache. It
fires under concurrency: two connections that both miss the same path both read
the file and both call `cache()`, and the loser's buffer leaks. Confirmed with
LeakSanitizer on a two-call repro.

Either free the content on the early return, or document that the caller keeps
ownership when the path was already present.

### 7. The logger is not format-checked, and one call is already wrong

> no test — asserting on log text would be brittle, and the compiler can do this

[`cache.c:58`](src/cache.c:58) reads `LOG_D("cached %s (hash %ul)", path, (unsigned long) page->hash)`.
`%ul` is `%u` followed by a literal `l`, and `%u` is then handed an `unsigned
long`. It prints a stray `l` after the number — visible in any DEBUG run as
`(hash 661261229l)` — and passing the wrong type is undefined behaviour that
happens to work on 64-bit little-endian.

The reason `-Wall -Wextra` stays quiet is that [`log_write()`](include/log.h:28)
is a plain variadic function. Giving it `__attribute__((format(printf, 4, 5)))`
makes the compiler check every `LOG_*` call site; confirmed that gcc then flags
this exact line. That is the real fix — it catches the next one too.

### 8. `read()` and `write()` return values are ignored

> no test — needs a client that stops reading mid-response, which is fiddly to
> make deterministic

Neither `write()` in [`response.c:54`](src/response.c:54) is checked, so a short
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
