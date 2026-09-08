#pragma once
#include "stdio.h"

/*
 * Measures an open file by seeking to its end.
 * fptr: an open, seekable stream. It is rewound to the start before return,
 *       so the caller can read it from the beginning.
 * Returns the size in bytes, or -1 if the position cannot be determined.
 */
long get_file_size(FILE *fptr);

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
