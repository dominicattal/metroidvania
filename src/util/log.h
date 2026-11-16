#ifndef ERROR_LOG_H
#define ERROR_LOG_H

#include <stdio.h>

typedef enum {
    FATAL = 0,
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    DEBUG = 4,
    TRACE = 5
} LogLevel;

#ifndef NDEBUG

    void _log(LogLevel severity, const char* message, const char* filename, int line, ...);

    #define log(severity, message, ...) \
        _log(severity, message, __FILE__, __LINE__, ##__VA_ARGS__)

    #define log_assert(condition, message, ...) \
        if (!(condition)) \
            _log(FATAL, message, __FILE__, __LINE__, ##__VA_ARGS__)
#else

    void _log(LogLevel severity, const char* message, ...);

    #define log(severity, message, ...) \
        _log(severity, message, ##__VA_ARGS__)

    #define log_assert(condition, message, ...) \
        if (!(condition)) \
            _log(FATAL, message, ##__VA_ARGS__)
#endif

#endif
