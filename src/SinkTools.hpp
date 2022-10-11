#pragma once
#include "LogRecord.hpp"
#include <cstdio>
namespace slog {
void format_severity(char* severity_str, int severity);
void format_time(char* time_str, unsigned long time, int seconds_precision=3);
void format_location(char* location_str, char const* file_name, int line_number);
int default_format(FILE* sink, LogRecord const* rec);
}
