#pragma once
#include "LogConfig.hpp"


// Syslog compatible severity
enum LogSeverity {
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


namespace slog {

struct LogRecord {

    LogRecord(long max_message_size_);

    void reset();

    LogRecord* capture(char const* filename, char const* function, int line,
                       int severity, int channel, const char* mtag, char const* format, ...);

    LogRecord* next;

    // Metadata
    char const* filename;
    char const* function;
    unsigned long time; // ns since Unix epoch
    unsigned long thread_id;
    int line;
    int severity;
    int channel;
    char const* tag;

    long message_max_size;
    char* message; // This must be the final member
};

constexpr int DEFAULT_CHANNEL = 0;
constexpr int NO_LINE = -1;

}
