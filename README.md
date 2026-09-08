# http-server-c

A small static HTTP/1.1 file server written in C. It listens on a TCP port, serves
files out of `site/`, spawns one detached thread per connection, and keeps page
bodies in an in-memory cache keyed by FNV-1a hash of the request path.

## Requirements

* A C compiler (`gcc` or `clang`) and `make`
* A POSIX system with pthreads (developed and tested on Linux)
* [k6](https://k6.io/) for the load test, optional

No external libraries are needed. The only vendored code is the FNV hash in `vendor/`.

## Build

```bash
make
```

That produces a `server` binary in the repo root. `make clean` removes the binary,
the object files, and the generated dependency files.

The build is warning-clean under `-Wall -Wextra`. Object files are compiled with
`-MMD -MP`, so headers are tracked as dependencies and touching one triggers the
right rebuilds.

## Run

The port is a required argument; there is no default.

```bash
./server 8080
```

The server prints a startup line to stdout and then logs every request to stderr:

```
Server listening on port 8080...

2026-09-08 21:42:30 [INFO] include/request.c:13: GET /index.html HTTP/1.1
2026-09-08 21:42:30 [DEBUG] include/handler.c:32: rendering ./site/index.html
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

```
server.c          socket setup, accept loop, thread spawn
include/
  http.[ch]       reads a request off the socket and parses the request line
  request.[ch]    the http_request struct and its logging
  handler.[ch]    maps a request path to a file under site/
  response.[ch]   loads the file and writes header plus body to the socket
  header.[ch]     renders the response header block
  cache.[ch]      in-memory page cache keyed by FNV-1a hash of the path
  utils.[ch]      whole-file reads
  log.h           header-only levelled logger with LOG_D/I/W/E macros
vendor/fnv.[ch]   FNV-1a hash
site/             the documents that get served
stress.js         k6 load test
```

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
./server 8080
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
roughly a third of requests are deliberate 404s and the `rate<0.01` threshold on
that metric is crossed on every run. It is not a useful pass/fail signal here.
Judge a run by the checks and by `hangs`.

For reference, a 30-second 50-user run on the current code gives around 31,000
iterations, 100% of checks passing, and a p(99) request duration near 200ms, with
a non-zero `hangs` count.

## Known limits

The server is deliberately small and has rough edges worth knowing about before
you lean on it:

* Client sockets are never closed after a response is written, so the process
  accumulates one file descriptor per request served and will eventually hit its
  `RLIMIT_NOFILE`. This is what a long or repeated stress run runs into.
* The listen backlog is 1, so connections arriving in a burst can be dropped
  before the accept loop reaches them.
* The page cache has no lock, and every connection is served on its own thread, so
  concurrent first-time requests race on it.
* Requests larger than 8192 bytes (`REQUEST_BUFFER_SIZE`) are truncated, and only
  the request line is parsed.
* Every response is labelled `text/html` regardless of the file being served.
