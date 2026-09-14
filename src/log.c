#include "log.h"
#include "color.h"
#include <stdio.h>
#include <stdbool.h>

log_level log_min = LOG_INFO;
FILE *log_file = NULL;
bool log_file_error = false;

void change_log_level(log_level level)
{
    log_min = level;
}

void log_write(log_level lvl, const char *file, int line,
               const char *fmt, ...)
{

    if (log_file == NULL)
    {
        log_file = fopen(LOG_PATH, "a");
        if (log_file == NULL)
        {
            log_file_error = true;
            LOG_E("Error opening logs");
        }
    }
    static const char *names[] = {"[DEBUG]" ANSI_RESET,
                                  "[INFO]" ANSI_RESET,
                                  "[WARN]" ANSI_RESET,
                                  "[ERROR]" ANSI_RESET};

    static const char *colors[] = {ANSI_BOLD ANSI_FG_YELLOW,
                                   ANSI_BOLD ANSI_FG_CYAN,
                                   ANSI_BOLD ANSI_FG_256(208),
                                   ANSI_BOLD ANSI_FG_RED};

    if (lvl < log_min)
        return;

    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    char ts[32];
    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tm);

    fprintf(stderr, "%s %s %s %s:%d: %s\n", colors[lvl], ts, names[lvl], file, line, msg);
    if (!log_file_error)
    {
        fprintf(log_file, "%s %s %s %s:%d: %s\n", colors[lvl], ts, names[lvl], file, line, msg);
        fflush(log_file);
    }
}