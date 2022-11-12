#pragma once
#include "LogConfig.hpp"
#include "LogRecord.hpp"
#include "LogRecordPool.hpp"
#include <functional>
#include <cstdio>

namespace slog {

class LogSink
{
public:
    virtual ~LogSink() = default;
    virtual void record(RecordNode const* node) = 0;
};

// Simple sink that ignores messages
class NullSink : public LogSink
{
public:
    void record(RecordNode const*) override { }
};


// Standard form for message formatters. This should
// write rec to sink and return the number of bytes
// written.
using Formatter = std::function<int (FILE* sink, RecordNode const* node)>;

/**
 * Writes the severity level as a four character string to severity_str
 */
void format_severity(char* severity_str, int severity);

/**
 * Format the time in ISO 8601/RFC 3339 format YYYY-MM-DD hh:mm:ss.fZ
 * where 
 *  (1) The timezone is always UTC
 *  (2) The number of fractional second digits f is controlled by 
 *      seconds_decimal_precision
 * The string should be at least 21 characters long for zero fractional seconds.
 * For the default form, 25 characters are required. 
 */
void format_time(char* time_str, unsigned long time, int seconds_decimal_precision=3);

/**
 * Makes a code location string. By default, this removes the path from file_name,
 * then appends the line number: "Foo.cpp@15"
 */
void format_location(char* location_str, char const* file_name, int line_number);

/**
 * A Formatter-compatible formatter that is used by ... well, by default
 */
int default_format(FILE* sink, RecordNode const* node);

}
