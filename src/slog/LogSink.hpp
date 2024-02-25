#pragma once
#include <cstdio>
#include <functional>

#include "LogConfig.hpp"
#include "LogRecord.hpp"

namespace slog {

/**
 * @brief Base class for all log sinks.
 *
 * To implement a new sink, only the record() function is required.
 */
class LogSink {
   public:
    virtual ~LogSink() = default;

    /// Save a record to a device.
    virtual void record(LogRecord const& node) = 0;
};

/// Simple sink that ignores messages
class NullSink : public LogSink {
   public:
    void record(LogRecord const&) override {}
};

/**
 * @brief Standard form for message formatters.
 * This should write rec to sink and return the number of bytes
 * written.
 */
using Formatter = std::function<int(FILE* sink, LogRecord const& node)>;

/**
 * @brief Writes the severity level as a four character string to severity_str
 */
void format_severity(char* severity_str, int severity);

/**
 * @brief Obtain a pointer to the null-terminated four character severity string
 */
char const* severity_string(int severity);

enum TimeFormatMode {
    FULL_SPACE,  // YYYY-MM-DD hh:mm:ss.fZ
    COMPACT,     // YYYYMMDDThhmmss.fZ
    FULL_T       // YYYY-MM-DDThh:mm:ss.fZ
};

/**
 * @brief Write an ISO 8601 timestamp to a string
 *
 * Format the time in ISO 8601/RFC 3339 format YYYY-MM-DD hh:mm:ss.fZ
 * where
 *  (1) The timezone is always UTC
 *  (2) The number of fractional second digits f is controlled by
 *      seconds_decimal_precision
 * @note The string should be at least 21 characters long for zero fractional seconds.
 * For the default form, 25 characters are required.
 *
 * If full_punctuation is true, then the format is YYYY-MM-DDThh:mm:ss.fZ
 */
void format_time(char* time_str, unsigned long time, int seconds_decimal_precision = 3,
                 TimeFormatMode format = FULL_SPACE);

/**
 * @brief Makes a code location string.
 *
 * This removes the path from file_name, then appends the line number: "Foo.cpp@15"
 * @note location_str must be at least size 64. Locations are truncated after 63 characters.
 */
void format_location(char* location_str, char const* file_name, int line_number);

/**
 * A Formatter-compatible formatter that is used by ... well, by default
 */
int default_format(FILE* sink, LogRecord const& node);

/**
 * A Formatter that just prints the records
 */
int no_meta_format(FILE* sink, LogRecord const& node);

}  // namespace slog
