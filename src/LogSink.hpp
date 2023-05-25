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
 * @brief Write an ISO8601 timestamp.
 *
 * Format the time in ISO 8601/RFC 3339 format YYYY-MM-DD hh:mm:ss.fZ
 * where
 *  (1) The timezone is always UTC
 *  (2) The number of fractional second digits f is controlled by
 *      seconds_decimal_precision
 * The string should be at least 21 characters long for zero fractional seconds.
 * For the default form, 25 characters are required.
 */
void format_time(char* time_str, unsigned long time, int seconds_decimal_precision = 3);

/**
 * @brief Makes a code location string.
 *
 * By default, this removes the path from file_name, then appends the line number: "Foo.cpp@15"
 */
void format_location(char* location_str, char const* file_name, int line_number);

/**
 * A Formatter-compatible formatter that is used by ... well, by default
 */
int default_format(FILE* sink, LogRecord const& node);

}  // namespace slog
