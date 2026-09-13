# Known issues

Two lists. **Bugs to fix** are defects — the code does something it did not mean
to, and each one is reproducible. **Accepted limits** are what the server
deliberately does not do; they are the cost of keeping it small, and they are not
on anybody's list to change.

Everything here was reproduced against the current code.

---

## Bugs to fix

Each one is reproducible. Where a test exists it fails today and goes green when
the bug is fixed; where there is none, the reason is given — usually that the
symptom is a leak, which shows up only under `-DSANITIZE=address`.

### 1. Caching a path twice leaks the body that loses

> no test — a leak does not change the response, so it takes
> `-DSANITIZE=address` to see. Reproduced by
> `cache::storing_a_path_twice_keeps_the_first_body` under ASan

`cache.h` says the cache takes ownership of `content`. When the path is already
present, [`cache()`](src/cache.c:41) returns early and drops the pointer without
freeing it, so nobody does.

Single-threaded this only fires if something stores the same path twice. Under
concurrency it fires on its own: two connections that both miss the same path both
read the file and both call `cache()`, and the loser's buffer leaks.

---

## Accepted limits

These are deliberate. The server is a development server; it is not trying to be
nginx, and none of the below is worth the code it would cost.

### Security

- Nothing keeps the served path inside `site/`. `global_req_handler()`
  concatenates the request path onto `SITE_DIR` and calls `stat`, so
  `GET /../server.c` returns the source file, and every regular file the process
  can read is reachable the same way.
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
