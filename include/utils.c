#include "utils.h"
#include <stdlib.h>
#include <sys/stat.h>


long get_file_size(FILE *fptr) {
    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    rewind(fptr);
    return size;
}


char *read_file(const char *path, size_t *out_len) {
    struct stat st;
    if (stat(path, &st) < 0 || !S_ISREG(st.st_mode)) return NULL;

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    char *buf = malloc(st.st_size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t n = fread(buf, 1, st.st_size, f);
    fclose(f);

    if (n != (size_t)st.st_size) { free(buf); return NULL; }

    buf[n] = '\0';
    *out_len = n;
    return buf;
}
