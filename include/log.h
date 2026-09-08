#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* Severity levels, ordered from least to most severe. */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level;

/* Lowest level that gets printed; anything below it is dropped. */
static log_level log_min = LOG_DEBUG;

/*
 * Formats and prints one log line. Use the LOG_* macros below rather than
 * calling this directly, since they fill in the file and line for you.
 * lvl:  severity of the message; nothing is printed if it is below log_min.
 * file: source file the message came from.
 * line: source line the message came from.
 * fmt:  printf-style format string, followed by its arguments.
 * Returns nothing. Writes one line to stderr, prefixed with a local
 * timestamp, the level name and the source location. Messages longer than
 * 1023 characters are truncated.
 */
static void log_write(log_level lvl, const char *file, int line,
                      const char *fmt, ...) {
    static const char *names[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    if (lvl < log_min) return;

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

    fprintf(stderr, "%s [%s] %s:%d: %s\n", ts, names[lvl], file, line, msg);
}

/* Log at one level, printf-style: LOG_I("%s %s", method, path). */
#define LOG_D(...) log_write(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_I(...) log_write(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_W(...) log_write(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_E(...) log_write(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
