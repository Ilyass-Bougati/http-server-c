#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level;

static log_level log_min = LOG_DEBUG;

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

#define LOG_D(...) log_write(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_I(...) log_write(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_W(...) log_write(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_E(...) log_write(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
