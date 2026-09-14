#pragma once
#include "stdio.h"

/*
 * Finds the suffix of the file named by `path`, including its dot.
 * path: the path to examine, NUL-terminated.
 * Returns a pointer into `path` at the dot, or NULL when there is no suffix:
 * no dot in the last path segment, a dot that opens the file name
 * (".gitignore" is a name, not a suffix), or a trailing dot. Nothing is
 * allocated, so nothing needs freeing -- and freeing the result would be a
 * free of an interior pointer. It stays valid as long as `path` does.
 */
char *get_file_suffix(char *path);

/*
 * Reads a whole file into memory.
 * path:    path of the file to read; must be an existing regular file.
 * out_len: set to the number of bytes read, not counting the trailing NUL.
 *          Left untouched when the call fails.
 * Returns a heap-allocated buffer holding the file contents plus a trailing
 * NUL, which the caller owns, or NULL if the file is missing, is not a
 * regular file, or could not be read in full.
 */
char *read_file(const char *path, size_t *out_len);

/*
 * Writes the startup banner -- the program name as ASCII art plus its version
 * -- to stderr.
 * Takes no arguments.
 * Returns nothing. Goes to stderr rather than stdout so that redirecting the
 * server's output does not capture it.
 */
void print_program_name();