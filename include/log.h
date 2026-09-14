#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_PATH "./logs/server.log"

/* Severity levels, ordered from least to most severe. */
typedef enum
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level;

/*
 * Sets the lowest level that gets written; anything below it is dropped.
 * level: the new minimum. LOG_DEBUG lets everything through, LOG_ERROR only
 *        the most severe.
 * Returns nothing. The level is one process-wide setting shared by every
 * source file, so call it once during start-up, before any connection thread
 * exists -- changing it while threads are logging is a data race.
 */
void change_log_level(log_level level);

/*
 * Formats and prints one log line. Use the LOG_* macros below rather than
 * calling this directly, since they fill in the file and line for you.
 * lvl:  severity of the message; nothing is printed if it is below log_min.
 * file: source file the message came from.
 * line: source line the message came from.
 * fmt:  printf-style format string, followed by its arguments.
 * Returns nothing. Writes one line to stderr, and the same line to LOG_PATH
 * when that file can be opened, both prefixed with a local timestamp, the
 * level name and the source location. Messages longer than 1023 characters
 * are truncated. LOG_PATH is relative, so it resolves against the working
 * directory the server was started from.
 */
void log_write(log_level lvl, const char *file, int line,
               const char *fmt, ...);

/* Log at one level, printf-style: LOG_I("%s %s", method, path). */
#define LOG_D(...) log_write(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_I(...) log_write(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_W(...) log_write(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_E(...) log_write(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
