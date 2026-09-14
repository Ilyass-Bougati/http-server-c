#include "utils.h"
#include <stdlib.h>
#include <sys/stat.h>
#include "color.h"
#include <string.h>

static char *version = "v1.0.1";

long get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    rewind(fptr);
    return size;
}

char *read_file(const char *path, size_t *out_len)
{
    struct stat st;
    if (stat(path, &st) < 0 || !S_ISREG(st.st_mode))
        return NULL;

    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    char *buf = malloc(st.st_size + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, st.st_size, f);
    fclose(f);

    if (n != (size_t)st.st_size)
    {
        free(buf);
        return NULL;
    }

    buf[n] = '\0';
    *out_len = n;
    return buf;
}

char *get_file_suffix(char *path)
{
    char *name = strrchr(path, '/');
    name = (name != NULL) ? name + 1 : path;
    char *dot = strrchr(name, '.');

    if (dot == NULL || dot == name || dot[1] == '\0')
    {
        return NULL;
    }

    return dot;
}

void lower_case(char *path)
{
    for (int i = 0; path[i] != '\0'; i++)
    {
        path[i] = path[i] <= 'z' && path[i] >= 'a'
                      ? path[i]
                      : path[i] + 'A' - 'a';
    }
}

void print_program_name()
{
    fprintf(stderr, ANSI_BOLD ANSI_FG_CYAN "\n\n"
                                           "███████╗███████╗███████╗ █████╗ ██╗   ██╗██╗  ████████╗██████╗ \n"
                                           "██╔════╝██╔════╝██╔════╝██╔══██╗██║   ██║██║  ╚══██╔══╝██╔══██╗\n"
                                           "███████╗█████╗  █████╗  ███████║██║   ██║██║     ██║   ██║  ██║\n"
                                           "╚════██║██╔══╝  ██╔══╝  ██╔══██║██║   ██║██║     ██║   ██║  ██║\n"
                                           "███████║███████╗██║     ██║  ██║╚██████╔╝███████╗██║   ██████╔╝\n"
                                           "╚══════╝╚══════╝╚═╝     ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝   ╚═════╝  %s\n\n\n" ANSI_RESET,
            version);
}