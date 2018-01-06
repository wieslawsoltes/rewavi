#include "common.h"

int rewavi_log(const char* name, log_t level, const char *message, ...)
{
    char *prefix;
    va_list args;

    va_start(args, message);

    switch(level)
    {
        case LOG_INFO:
            prefix = "info";
            break;
        case LOG_WARNING:
            prefix = "warn";
            break;
        case LOG_ERROR:
            prefix = "error";
            break;
        default:
            prefix = "unknown";
            break;
    }

    fprintf(stderr, "%s [%s]: ", name, prefix);
    vfprintf(stderr, message, args);

    va_end(args);
    return level;
}