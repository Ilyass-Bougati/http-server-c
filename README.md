# http-server-c

A small static HTTP/1.1 file server written in C. It listens on a TCP port, serves
files out of `site/`, spawns one detached thread per connection, and keeps page
bodies in an in-memory cache keyed by FNV-1a hash of the request path.

It is a development server. [KNOWN_ISSUES.md](KNOWN_ISSUES.md) lists both the open
bugs and the limits that are deliberate — do not expose it to a network you do not
control.

Everything beyond running it — sanitizer builds, editor setup, the source layout and
the k6 load test — is in [DEVELOPMENT.md](DEVELOPMENT.md).

## Run with Docker

### Pull the published image

Images are published to GitHub Container Registry from `v*` tags. `latest` tracks
the newest release, and each release also gets its version as a tag (`1.0.0`).

```bash
docker run --rm -p 8080:8080 ghcr.io/ilyass-bougati/http-server-c:latest
```

To serve your own pages without rebuilding, mount a directory over `/site`:

```bash
docker run --rm -p 8080:8080 -v "$PWD/site:/site" ghcr.io/ilyass-bougati/http-server-c:latest
```

### Build the image yourself

```bash
docker build -t http-server-c .
```

```bash
docker run --rm -p 8080:8080 http-server-c
```

## Build from source and run

Needs a C compiler (`gcc` or `clang`), [CMake](https://cmake.org/) 3.16 or newer, a
build tool for it (`make` or `ninja`), and Linux. No external libraries; the only
vendored code is the FNV hash in `vendor/`.

Configure once, then build. Everything generated lands in `build/`, which is ignored
by git; deleting that directory is the full clean.

```bash
cmake -S . -B build
```

```bash
cmake --build build
```

That produces `build/server`. The port is a required argument, there is no default,
and `site/` is resolved against the working directory — so start it from the project
root, not from inside `build/`:

```bash
./build/server 8080
```

There is also a `run` target that builds first and sets the working directory for
you, hardcoded to port 8080:

```bash
cmake --build build --target run
```

`CMAKE_BUILD_TYPE` defaults to `Debug`. For an optimised binary, configure a second
directory rather than overwriting the first:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release && cmake --build build-release
```

## Check that it works

```bash
curl -i http://localhost:8080/
```

The server prints a startup line to stdout and then logs every request to stderr:

```
Server listening on port 8080...

2026-09-13 20:50:09 [DEBUG] src/http.c:13: thread start, tid 2077185
2026-09-13 20:50:09 [INFO] src/request.c:13: GET / HTTP/1.
2026-09-13 20:50:09 [DEBUG] src/handler.c:24: rendering index.html for /
2026-09-13 20:50:09 [DEBUG] src/cache.c:58: cached ./site/index.html (hash 661261229l)
```

Source paths in those lines come from `__FILE__`, so they are absolute when CMake was
pointed at an absolute source directory; they are shortened above.

Log verbosity is controlled by `log_min` in `include/log.h`, which defaults to
`LOG_DEBUG`. Raise it to `LOG_INFO` to quiet the per-request rendering lines.

Routing rules:

| Request path  | Served file                                     | Status |
| ------------- | ----------------------------------------------- | ------ |
| `/`           | `site/index.html`                               | 200    |
| `/<name>`     | `site/<name>` if it is an existing regular file | 200    |
| anything else | `site/not_found.html`                           | 404    |

Every response is sent as `text/html` with a `Content-Length` and
`Connection: close`. Only the request line is parsed; request headers are read off
the socket but ignored.

Add pages by dropping files into `site/`. They are picked up on the next request for
that path, and the body is cached in memory from the first hit onward, so restart the
server after editing a file you have already requested.

## Run the tests

The suite uses [Criterion](https://github.com/Snaipe/Criterion). It is a test-only
dependency — it never gets linked into the server, and a tree without it builds and
runs exactly as before, just with the tests skipped.

```bash
sudo apt-get install libcriterion-dev
```

Tests are picked up by the ordinary configure step and built alongside the server,
then run through CTest:

```bash
cmake -S . -B build && cmake --build build
```

```bash
ctest --test-dir build --output-on-failure
```

Each of the three suites is also a binary you can run on its own, which is the
quicker loop while working on one of them:

```bash
./build/test/test_serve --verbose
```

Criterion takes `--list` to show the cases and `--filter` to pick them. The filter
separates suite from test with a slash, even though the output prints them with
`::`:

```bash
./build/test/test_serve --filter 'serve/a_query*'
```

The suite is green in a normal build. Some of the bugs still open in
[KNOWN_ISSUES.md](KNOWN_ISSUES.md) only show up under a sanitizer, and the entry for
each says which build surfaces it — so a green `ctest` is a weaker statement than it
looks. Criterion runs every test in its own process, so a test that crashes is
reported as a single `CRASH` and the rest of the suite still runs.

Configuring with `-DSANITIZE=address` builds the tests sanitized too, which is how
the leaks in that list show up; see [DEVELOPMENT.md](DEVELOPMENT.md). To leave the
tests out of the build entirely, configure with `-DBUILD_TESTING=OFF`.
