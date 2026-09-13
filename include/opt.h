#pragma once
#include <getopt.h>
#include <stdlib.h>

static const struct option opts[] = {
    {"port", required_argument, NULL, 'p'},
    {"log", required_argument, NULL, 'l'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

static void usage(char *program)
{
    printf("Usage %s\n", program);
    printf("\t-p --port\tThe port the application should run in\n");
    printf("\t-l --log \tThe log level (DEBUG, INFO, WARN, ERROR)\n");
    printf("\t-h --help\tHelp\n");
}