# http-server-c

A small static HTTP/1.1 file server written in C. It listens on a TCP port, serves
files out of `site/`, spawns one detached thread per connection, and keeps page
bodies in an in-memory cache keyed by FNV-1a hash of the request path.

## Requirements

* A C compiler (`gcc` or `clang`), [CMake](https://cmake.org/) 3.16 or newer,
  and a build tool for it (`ninja` or `make`)
* A POSIX system with pthreads (developed and tested on Linux)
* [k6](https://k6.io/) for the load test, optional

No external libraries are needed. The only vendored code is the FNV hash in `vendor/`.

## Build

Configure once, then build. Everything generated lands in `build/`, which is
ignored by git; deleting that directory is the full clean.

```bash
cmake -S . -B build
```

```bash
cmake --build build
```

That produces `build/server`. Pass `-G Ninja` to the configure step for a faster
incremental build if you have ninja installed. After the first configure, only the
build command is needed; CMake re-runs itself when `CMakeLists.txt` changes, and
the source list uses `CONFIGURE_DEPENDS`, so a new `.c` dropped into `src/` or
`vendor/` is picked up without reconfiguring by hand.

The build is warning-clean under `-Wall -Wextra`. Header dependencies are tracked
automatically, so touching a header rebuilds exactly what depends on it.

`CMAKE_BUILD_TYPE` defaults to `Debug`. For an optimised binary, configure a
second directory rather than overwriting the first:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release && cmake --build build-release
```

### Editor integration

The configure step writes `build/compile_commands.json`, which is how clangd and
the VS Code C/C++ extension learn that headers live in `include/`. Without it they
cannot resolve `#include "cache.h"` from a file in `src/`, and every symbol
declared in that header shows up as an error even though the build is clean.

clangd finds the file on its own if you point it at the build directory, or symlink
it to where clangd looks by default:

```bash
ln -sf build/compile_commands.json compile_commands.json
```

### Sanitizers

`-DSANITIZE=` builds with a sanitizer attached: `address`, `thread`, `undefined`,
or `none` (the default). ThreadSanitizer is the one worth reaching for here, since
the page cache is written without a lock and TSan reports that race directly.
Sanitizers cannot be combined, so use one build directory per sanitizer:

```bash
cmake -S . -B build-tsan -DSANITIZE=thread && cmake --build build-tsan
```

Then drive it with the k6 burst scenario below and read the report on stderr.

## Run

The port is a required argument; there is no default. The server resolves `site/`
against its working directory, so start it from the project root, not from inside
`build/`.

```bash
./build/server 8080
```

There is also a `run` target that builds first and sets the working directory for
you, hardcoded to port 8080:

```bash
cmake --build build --target run
```

The server prints a startup line to stdout and then logs every request to stderr:

```
Server listening on port 8080...

2026-09-08 21:42:30 [INFO] src/request.c:13: GET /index.html HTTP/1.1
2026-09-08 21:42:30 [DEBUG] src/handler.c:32: rendering ./site/index.html
```

Log verbosity is controlled by `log_min` in `include/log.h`, which defaults to
`LOG_DEBUG`. Raise it to `LOG_INFO` to quiet the per-request rendering lines.

Check it with curl:

```bash
curl -i http://localhost:8080/
```

Routing rules:

| Request path | Served file | Status |
|---|---|---|
| `/` | `site/index.html` | 200 |
| `/<name>` | `site/<name>` if it is an existing regular file | 200 |
| anything else | `site/not_found.html` | 404 |

Every response is sent as `text/html` with a `Content-Length` and
`Connection: close`. Only the request line is parsed; request headers are read
off the socket but ignored.

Add pages by dropping files into `site/`. They are picked up on the next request
for that path, and the body is cached in memory from the first hit onward, so
restart the server after editing a file you have already requested.

## Layout

Headers live in `include/`, implementation in `src/`. Each `.c` in `src/` pairs
with the header of the same name.

```
CMakeLists.txt    build definition
server.c          socket setup, accept loop, thread spawn
include/ + src/
  http            reads a request off the socket and parses the request line
  request         the http_request struct and its logging
  handler         maps a request path to a file under site/
  response        loads the file and writes header plus body to the socket
  header          renders the response header block
  cache           in-memory page cache keyed by FNV-1a hash of the path
  utils           whole-file reads
  log.h           header-only levelled logger, no .c of its own
vendor/fnv.[ch]   FNV-1a hash
site/             the documents that get served
stress.js         k6 load test
build/            generated; not in git
```

Both `include/` and the project root are on the include path, so a source file
reaches a sibling header as `#include "cache.h"` and the vendored hash as
`#include "vendor/fnv.h"`, with no relative `../` paths.

Each header carries a comment above every prototype describing what the function
does, what each argument means, and what comes back, including which pointers the
caller is expected to free.

## Load testing with k6

`stress.js` drives the server with mixed 200 and 404 traffic and asserts on the
correctness of each response, not just on throughput. Every third iteration asks
for a missing page, so the 404 branch gets exercised as hard as the happy path.

Install k6 first if you do not have it. On Debian or Ubuntu, follow the
[official install docs](https://grafana.com/docs/k6/latest/set-up/install-k6/);
`k6 version` should print a version once it is on your PATH.

Start the server in one terminal, then run the test in another:

```bash
./build/server 8080
```

```bash
k6 run --vus 50 --duration 30s stress.js
```

That is 50 concurrent virtual users hammering the server for 30 seconds.

To point the test at a different host or port, set `BASE`:

```bash
BASE=http://localhost:9000 k6 run --vus 50 --duration 30s stress.js
```

### The two run modes

`stress.js` defines its own `scenarios` block, and k6 resolves the two forms
differently:

* `k6 run --vus 50 --duration 30s stress.js` replaces the script's scenarios with
  a single flat 50-user, 30-second run. k6 says so on startup with
  `"cli" level configuration overrode scenarios configuration entirely`. This is
  the steady-load run and the one to reach for first.
* `k6 run stress.js` uses the scenarios in the file: the same 30 seconds of steady
  load, followed by a 30-second burst phase that opens connections at a fixed 200
  per second regardless of whether the server is keeping up. The burst phase is
  what surfaces accept-loop and shared-state races, so use it when you are chasing
  a concurrency bug rather than measuring baseline behaviour.

Thresholds in the file apply to both forms.

### Reading the results

Four checks run against every response:

* `status correct`, 200 on a real page and 404 on a missing one
* `body non-empty`
* `length matches body`, the declared `Content-Length` equals the bytes actually
  received, which is the one that catches a truncated or over-declared response
* `content-type present`

A clean run has `checks_succeeded` at 100% and `hangs` at 0. The `hangs` counter is
custom: it counts requests that timed out at 3 seconds or returned no status at
all, which is the signature of a connection the server accepted but never
finished with.

One caveat on `http_req_failed`. k6 counts any non-2xx response as a failed
request, and this script asks for a missing page on one iteration in three, so
the metric sits at roughly 33% on a perfectly healthy run. The threshold is
`rate<0.34` to accommodate that, which means it only fires on failures well past
the deliberate 404s. Judge a run by the checks and by `hangs`.

For reference, a 30-second 50-user run on the current code gave around 118,000
requests at roughly 3,900 per second, 100% of checks passing, `hangs` at 0, and
the server still alive at the end. Throughput and latency swing a lot between
runs, since k6 and the server share the same cores and every request writes a
log line.

## Known limits

The server is deliberately small, and the list below is what breaks when you
lean on it. All of it was reproduced against the current code. Treat it as a
development server and do not expose it.

### Ways to kill the process

* A file that passes `stat` but will not open poisons the cache. `read_file()`
  returns NULL, and `send_http_static_page_response()` neither checks it nor
  initialises `out_len`: the first request answers 200 with a garbage length and
  caches NULL, and the second segfaults in `strlen(NULL)`.
* A connection that closes without sending is served from uninitialised memory.
  `read()`'s return value is unchecked and `init_request()` does not zero its
  allocation, so `sscanf` matches nothing and the server logs and routes on heap
  garbage.

### Security

* Nothing keeps the served path inside `site/`. `global_req_handler()`
  concatenates the request path onto `SITE_DIR` and calls `stat`, so
  `GET /../server.c` returns the source file, and every regular file the process
  can read is reachable the same way.
* Nothing resists a client trying to be expensive: no request rate limit, no
  timeouts, no cap on threads.

### Concurrency

* The page cache has no lock. Every connection runs on its own thread, and
  `cache()` reallocs `__site_cache` and increments `__cache_size`
  unsynchronised, so concurrent first-time requests for an uncached path race on
  the array.
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
  so lookups compare hashes and a collision serves the wrong body. Entries are
  never invalidated or freed, which is why editing an already-requested file
  needs a restart.
* Every 404 leaks its path buffer: `global_req_handler()` overwrites the
  `calloc`ed `path` with `NOT_FOUND_PATH` before rendering, and never frees it.
