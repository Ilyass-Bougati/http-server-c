#pragma once
#include "stdio.h"

long get_file_size(FILE *fptr);

char *read_file(const char *path, size_t *out_len);
