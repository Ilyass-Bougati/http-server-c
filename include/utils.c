#include "utils.h"

long get_file_size(FILE *fptr) {
    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    rewind(fptr);
    return size;
}
