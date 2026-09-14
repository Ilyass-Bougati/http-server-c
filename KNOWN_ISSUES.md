# Known issues

Two lists. **Performance** is work the server does per request that it does not
have to; none of it is wrong, and none of it matters until the thing is under
load. **Accepted limits** are what the server deliberately does not do — the cost
of keeping it small, and not on anybody's list to change.

There is no open-bugs list at the moment, which is the good kind of empty. When
one comes back, it goes above Performance, and the test that fails for it gets
named in its entry.

---

## Performance

None of the below is a correctness problem, and none of it matters at two files
and one visitor. They are listed because they are the things that decide how the
server behaves under the k6 burst, and because most are cheap to fix.

**On the reported regression:** I could not reproduce it as a measurement. The
one load-independent number, syscalls per request, went *down* rather than up
(6 writes per request before, 4 now, because the default level moved from DEBUG
to INFO). Throughput runs on a loaded machine gave ratios from 0.84 to 1.29 across
identical repeats, which is noise, not signal. What follows is therefore read off
the code rather than measured, ordered by how much per-request work it adds. To
get a real number, run the k6 steady scenario on an idle machine against two
builds, alternating between them rather than one after the other.

### Path normalisation rebuilds the path one segment at a time

Added in "Fixing a bug of reaching outside site/", and the largest new cost on the
request path. [`parse_request()`](src/http.c:53) allocates a 1-byte buffer, then for
every path segment does a `realloc` plus two `strcat`s. `strcat` rescans the whole
buffer from the start each time, so a path of k segments costs O(k) reallocs and
O(k²) scanning. The copy loop underneath it then calls `strlen(buff)` in the loop
condition, which is another O(n²).

Both are avoidable without changing behaviour: the normalised path is never longer
than the input, so a single buffer of the input's length can be allocated once and
written with a moving cursor, and `strlen` can be hoisted out of the loop.

### Roughly 13 KB is zeroed per request, and 8 allocations are made

`calloc` zeroes everything it hands back. Per request, in order:

| Where | Bytes | Zeroed? |
|---|---|---|
| [http.c:25](src/http.c:25) request buffer (`REQUEST_BUFFER_SIZE`) | 8192 | yes |
| [http.c:33](src/http.c:33) scratch path | 2048 | yes |
| [http.c:53](src/http.c:53) normalisation buffer | grows | realloc'd per segment |
| [request.c:7](src/request.c:7) `http_request` | ~2080 | yes |
| [handler.c:41](src/handler.c:41) site path | path length | yes |
| [response.c:14](src/response.c:14) response struct | 16 | yes |
| [header.c:13](src/header.c:13) header block | 1024 | yes |
| [cache.c:89](src/cache.c:89) cache-hit handle | 24 | yes |

The request buffer and the `http_request` are the two worth attention: both are
fixed-size, both are function-local in lifetime, and both could live on the
connection thread's stack instead, removing two allocations and ~10 KB of
memset per request. Only the bytes actually read need clearing, not all 8192.

### The cache hit allocates, copies, and frees to return three fields

[`get_cached()`](src/cache.c:89) `calloc`s a `site_page`, `memcpy`s the entry into
it, and hands it back for the caller to free. That is a malloc/free pair per
request to return a hash, a pointer and a length.

Entries are never evicted or freed, so a cached `site_page *` stays valid for the
life of the process. Returning `const site_page *` straight out of the table would
be safe and would delete the allocation, the copy and the free.

### Every request serialises on one mutex and scans the whole cache

[`get_cached()`](src/cache.c:86) takes a single global mutex and walks the array
comparing hashes. That is O(N) in cached pages, under a lock every request must
queue for, so cache lookups cannot overlap no matter how many cores are free.

At two files this is invisible. It is the structure, not the size, that caps
concurrency: a hash table would make the scan O(1), and a read-write lock (or a
table that is only ever appended to) would let lookups run in parallel.

### Each log line is flushed to disk on its own

[`log_write()`](src/log.c) calls `fflush(log_file)` after every line, which turns
each line into its own `write` syscall instead of letting stdio batch them. A
ten-second run at 20 connections produced a 12 MB log file. Flushing per line is
the right default only if the process is expected to die without unwinding;
otherwise flushing on a timer, or leaving it to stdio, costs nothing in practice.

The same `FILE *` is written by every connection thread, and glibc takes an
internal lock per stdio call, so threads also serialise here -- twice per line,
once for stderr and once for the file.

### Two writes per response where one would do

The header and the body go out as separate `write` calls
([response.c:61](src/response.c:61) and [:66](src/response.c:66), and again at
[:119](src/response.c:119)/[:124](src/response.c:124)). One `writev` with a
two-entry iovec sends both in a single syscall, and avoids handing the kernel a
small header segment on its own.

### `copy_content` copies a byte at a time

[`copy_content()`](src/cache.c:24) fills the cache buffer with a `for` loop over
single bytes where `memcpy` would do -- which the compiler cannot always turn back
into a vectorised copy. It runs once per cached path rather than per request, so
it only shows up as a slower first hit on a large file.

---

## Accepted limits

These are deliberate. The server is a development server; it is not trying to be
nginx, and none of the below is worth the code it would cost.

### Security

- Nothing resists a client trying to be expensive: no request rate limit, no
  timeouts, no cap on threads.

**Do not expose this to a network you do not control.**

### Concurrency

- Reads and writes have no timeout, so a client that connects and never sends
  holds a thread until it goes away on its own.
- Every connection gets a detached thread with no ceiling, so what limits load
  is thread creation rather than anything the server decides.

### Protocol and caching

- Requests over 8192 bytes (`REQUEST_BUFFER_SIZE`) are truncated, only the
  request line is parsed, and one `read()` per connection means a request line
  split across TCP segments is never reassembled.
- The method is parsed and then ignored: `POST /index.html` gets the same 200
  and the same body as `GET`.
- Every response is labelled `text/html` whatever the file holds, the reason
  phrase is always empty, and `Connection: close` is the only mode.
- Cache entries are keyed by a 32-bit FNV-1a hash and the path is never stored,
  so lookups compare hashes and a collision serves the wrong body. Handling
  collisions would mean a bucket list per entry, which is not worth it here.
- Cache entries are never invalidated or freed, which is why editing an
  already-requested file needs a restart.
- Request paths are used exactly as they arrive, with no percent-decoding, so a
  file whose name needs escaping is unreachable. Decoding is what would turn
  `%2e%2e%2f` into `../`, which is why it is left alone deliberately.
