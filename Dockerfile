FROM debian:bookworm AS build
RUN apt-get update && apt-get install -y --no-install-recommends build-essential cmake
WORKDIR /src
COPY . .
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-static"
RUN cmake --build build

FROM scratch
WORKDIR /
COPY --from=build /src/build/server /usr/local/bin/server
COPY site /site
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/server", "8080"]