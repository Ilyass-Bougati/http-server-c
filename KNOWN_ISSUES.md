# Known issues

The server is deliberately small, and the list below is what breaks when you lean on
it. All of it was reproduced against the current code. Treat it as a development
server and do not expose it.

## Security

* Nothing keeps the served path inside `site/`. `global_req_handler()`
  concatenates the request path onto `SITE_DIR` and calls `stat`, so
  `GET /../server.c` returns the source file, and every regular file the process
  can read is reachable the same way.
* Nothing resists a client trying to be expensive: no request rate limit, no
  timeouts, no cap on threads.

## Concurrency

* Reads and writes have no timeout, so a client that connects and never sends
  holds a thread until it goes away on its own.
* Every connection gets a detached thread with no ceiling, so what limits load
  is thread creation rather than anything the server decides.

## Protocol and caching

* Requests over 8192 bytes (`REQUEST_BUFFER_SIZE`) are truncated, only the
  request line is parsed, and one `read()` per connection means a request line
  split across TCP segments is never reassembled.
* The method is parsed and then ignored: `POST /index.html` gets the same 200
  and the same body as `GET`.
* Every response is labelled `text/html` whatever the file holds, the reason
  phrase is always empty, and `Connection: close` is the only mode.
* Cache entries are keyed by a 32-bit FNV-1a hash and the path is never stored,
  so lookups compare hashes and a collision serves the wrong body. Entries are
  never invalidated or freed, which is why editing an already-requested file
  needs a restart.
* Every 404 leaks its path buffer: `global_req_handler()` overwrites the
  `calloc`ed `path` with `NOT_FOUND_PATH` before rendering, and never frees it.
