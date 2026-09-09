cmake_minimum_required(VERSION 3.16)

project(http-server-c
    VERSION 0.1.0
    DESCRIPTION "A small static HTTP/1.1 file server"
    LANGUAGES C)

# Always emit compile_commands.json. Editors and clang-tidy read it to learn the
# include paths, which is what stops a header in include/ from showing up as an
# unresolved symbol in the editor.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# This is a development server, so default to a debuggable build. Override per
# build directory with -DCMAKE_BUILD_TYPE=Release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# CONFIGURE_DEPENDS re-runs the glob when the build is invoked, so dropping a new
# .c into src/ or vendor/ is picked up without re-running cmake by hand. This
# keeps the "just add a file" behaviour the old makefile's $(wildcard) had.
file(GLOB APP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/*.c")

add_executable(server
    server.c
    ${APP_SOURCES})

target_compile_features(server PRIVATE c_std_11)

# include/ makes "cache.h" resolve from any source file; the project root makes
# "vendor/fnv.h" resolve without a ../ climb out of the including file's dir.
target_include_directories(server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_options(server PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
    $<$<C_COMPILER_ID:MSVC>:/W4>)

# Threads::Threads adds -pthread at both compile and link, rather than relying on
# glibc 2.34+ folding pthread into libc.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)

# -DSANITIZE=address|thread|undefined|none (default none). ThreadSanitizer is the
# one that finds the unsynchronised page cache; it cannot be combined with
# address, so pick one at a time.
set(SANITIZE "none" CACHE STRING "Sanitizer to build with")
set_property(CACHE SANITIZE PROPERTY STRINGS none address thread undefined)

if(NOT SANITIZE STREQUAL "none")
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "SANITIZE=${SANITIZE} needs gcc or clang")
    endif()
    message(STATUS "Building with -fsanitize=${SANITIZE}")
    target_compile_options(server PRIVATE
        -fsanitize=${SANITIZE} -fno-omit-frame-pointer -g)
    target_link_options(server PRIVATE -fsanitize=${SANITIZE})
endif()

# The server resolves SITE_DIR ("./site") against its working directory, so it has
# to be started from the project root. This target does that for you:
#     cmake --build build --target run
add_custom_target(run
    COMMAND "$<TARGET_FILE:server>" 8080
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
    DEPENDS server
    COMMENT "Serving ${CMAKE_CURRENT_SOURCE_DIR}/site on port 8080")
