#pragma once
#include "LogRecord.hpp"
#include <cstdio>
#include <functional>

namespace slog {
    
// Standard form for message formatters. This should
// write rec to sink and return the number of bytes
// written.
using Formatter = std::function<int (FILE* sink, LogRecord const& rec)>;

void format_severity(char* severity_str, int severity);
void format_time(char* time_str, unsigned long time, int seconds_decimal_precision=3);
void format_location(char* location_str, char const* file_name, int line_number);
int default_format(FILE* sink, LogRecord const& rec);
}
