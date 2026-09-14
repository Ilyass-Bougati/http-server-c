#pragma once
#include <getopt.h>
#include <stdlib.h>

/*
 * The long options getopt_long accepts, terminated by the all-zero entry it
 * requires. Each maps to the short option of the same letter, so --port and -p
 * are the same flag.
 */
static const struct option opts[] = {
    {"port", required_argument, NULL, 'p'},
    {"log", required_argument, NULL, 'l'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

/*
 * Prints the list of accepted options.
 * program: the program name to show in the usage line, normally argv[0].
 * Returns nothing; output goes to stdout, since it is asked for with --help
 * rather than being an error.
 */
static void usage(char *program)
{
    printf("Usage %s\n", program);
    printf("\t-p --port\tThe port the application should run in\n");
    printf("\t-l --log \tThe log level (DEBUG, INFO, WARN, ERROR)\n");
    printf("\t-h --help\tHelp\n");
}