#include "log.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdarg.h>

static int level = DEBUG;

#ifndef NDEBUG
static const char* severity_string(LogLevel severity)
{
    switch (severity) {
        case FATAL: return "\033[31mFATAL\033[0m";
        case ERROR: return "\033[38;2;255;165;0mERROR\033[0m";
        case WARNING: return "\033[33mWARNING\033[0m";
        case INFO: return "\033[32mINFO\033[0m";
        case DEBUG: return "\033[34mDEBUG\033[0m";
        case TRACE: return "\033[38;2;128;68;255mTRACE\033[0m";
        default: break;
    }
    return "";
}
#else
static const char* severity_string(LogLevel severity)
{
    switch (severity) {
        case FATAL: return "FATAL";
        case ERROR: return "ERROR";
        case WARNING: return "WARNING";
        case INFO: return "INFO";
        case DEBUG: return "DEBUG";
        case TRACE: return "TRACE";
        default: break;
    }
    return "";
}
#endif

static void get_timestamp(int* Y, int* M, int* D, int* h, int* m, int* s)
{
    time_t cur_time;
    struct tm local_time;
    cur_time = time(NULL);
    local_time = *localtime(&cur_time);
    *Y = local_time.tm_year + 1900;
    *M = local_time.tm_mon + 1;
    *D = local_time.tm_mday;
    *h = local_time.tm_hour;
    *m = local_time.tm_min;
    *s = local_time.tm_sec;
}

#ifndef NDEBUG
void _log(LogLevel severity, const char* message, const char* filename, int line, ...)
{
    if ((int)severity > level)
        return;
    int Y, M, D, h, m, s;
    va_list args;
    get_timestamp(&Y, &M, &D, &h, &m, &s);
    fprintf(stderr, "\033[35;2;70;140;70m%02d:%02d:%02d %s \033[96m%s:%d\033[0m ", 
            h, m, s, 
            severity_string(severity), 
            filename, line);
    va_start(args, line);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    if (severity == FATAL)
        abort();
}
#else
void _log(LogLevel severity, const char* message, ...)
{
    int Y, M, D, h, m, s;
    va_list args;
    get_timestamp(&Y, &M, &D, &h, &m, &s);
    fprintf(stderr, "[%4d-%02d-%02d %02d:%02d:%02d][%s] ", 
            Y, M, D, h, m, s, 
            severity_string(severity));
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);
    if (severity == FATAL)
        abort();
}
#endif
