#pragma once

#ifndef SLOG_MAX_CHANNEL
#define SLOG_MAX_CHANNEL 4
#endif

#ifndef SLOG_LOGGING_ENABLED
#define SLOG_LOGGING_ENABLED 1
#endif

#ifndef SLOG_MESSAGE_SIZE
#define SLOG_MESSAGE_SIZE 1024
#endif

#ifndef SLOG_POOL_SIZE
#define SLOG_POOL_SIZE 1048576
#endif


#define SLOG_DUMP_TO_CONSOLE_WHEN_STOPPED

// #define SLOG_LOCK_FREE

namespace slog
{
    
constexpr int DEFAULT_CHANNEL = 0;

constexpr unsigned long TAG_SIZE = 16;
    
// Syslog compatible severity levels
enum LogSeverity : int {
    FATL = -100,
    EMER = 0,
    ALRT = 100,
    CRIT = 200,
    ERRR = 300,
    WARN = 400,
    NOTE = 500,
    INFO = 600,
    DBUG = 700
};

}
