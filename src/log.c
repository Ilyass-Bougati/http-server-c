#include "log.h"
#include "color.h"

log_level log_min = LOG_INFO;

void change_log_level(log_level level)
{
    log_min = level;
}

void log_write(log_level lvl, const char *file, int line,
               const char *fmt, ...)
{
    static const char *names[] = {ANSI_BOLD ANSI_FG_YELLOW "[DEBUG]" ANSI_RESET,
                                  ANSI_BOLD ANSI_FG_CYAN "[INFO]" ANSI_RESET,
                                  ANSI_BOLD ANSI_FG_256(208) "[WARN]" ANSI_RESET,
                                  ANSI_BOLD ANSI_FG_RED "[ERROR]" ANSI_RESET};

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

    fprintf(stdout, "%s %s %s %s:%d: %s\n", colors[lvl], ts, names[lvl], file, line, msg);
}