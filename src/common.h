#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <vfw.h>

#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT 3
#endif

#define VERSION "0.03"

typedef enum {
    LOG_INFO,
    LOG_ERROR,
    LOG_WARNING
} log_t;

inline int rewavi_log(const char* name, log_t level, const char *message, ...)
{
    char *prefix;
    va_list args;
    va_start(args, message);
    switch (level)
    {
    case LOG_INFO:    prefix = "info";    break;
    case LOG_WARNING: prefix = "warn";    break;
    case LOG_ERROR:   prefix = "error";   break;
    default:          prefix = "unknown"; break;
    }
    fprintf(stderr, "%s [%s]: ", name, prefix);
    vfprintf(stderr, message, args);
    va_end(args);
    return level;
}

