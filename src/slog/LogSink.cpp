#include "LogSink.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "ConsoleSink.hpp"

namespace slog {

int default_format(FILE* sink, LogRecord const& rec)
{
    char severity_str[16];
    char time_str[32];
    format_severity(severity_str, rec.meta.severity);
    format_time(time_str, rec.meta.time, 3);
    int count = fprintf(sink, "[%s %s %s] %s", severity_str, rec.meta.tag, time_str, rec.message);
    for (LogRecord const* more = rec.more; more != nullptr; more = more->more) {
        count += fprintf(sink, "%s", more->message);
    }
    return count;
}

void format_severity(char* severity_str, int severity)
{
    if (severity >= DBUG) {
        strncpy(severity_str, "DBUG", 5);
    } else if (severity >= INFO) {
        strncpy(severity_str, "INFO", 5);
    } else if (severity >= NOTE) {
        strncpy(severity_str, "NOTE", 5);
    } else if (severity >= WARN) {
        strncpy(severity_str, "WARN", 5);
    } else if (severity >= ERRR) {
        strncpy(severity_str, "ERRR", 5);
    } else if (severity >= CRIT) {
        strncpy(severity_str, "CRIT", 5);
    } else if (severity >= ALRT) {
        strncpy(severity_str, "ALER", 5);
    } else if (severity >= EMER) {
        strncpy(severity_str, "EMER", 5);
    } else {
        strncpy(severity_str, "FATL", 5);
    }
}

void format_time(char* time_str, unsigned long t, int seconds_precision)
{
    constexpr unsigned long NANOS_PER_SEC = 1000000000ULL;
    if (seconds_precision < 0) { seconds_precision = 0; }
    if (seconds_precision > 9) { seconds_precision = 9; }

    const int min_size = 18;
    time_t seconds_since_epoch = t / NANOS_PER_SEC;
    unsigned long nano_remainder = t % NANOS_PER_SEC;

    tm broken;
    gmtime_r(&seconds_since_epoch, &broken);

    // Most of the iso timestamp
    auto offset = strftime(time_str, min_size, "%Y-%m-%d %H:%M:", &broken);

    // Seconds and zone
    double secondsPart = static_cast<double>(broken.tm_sec) + 1e-9 * nano_remainder;
    int seconds_width = (seconds_precision == 0 ? 2 : 3 + seconds_precision);
    sprintf(time_str + offset, "%0*.*fZ", seconds_width, seconds_precision, secondsPart);
}

void format_location(char* location_str, char const* file_name, int line_number)
{
    snprintf(location_str, 64, "%s@%d", basename(file_name), line_number);
}

void ConsoleSink::record(LogRecord const& rec)
{
    mformat(stdout, rec);
    fputc('\n', stdout);
    fflush(stdout);
}

}  // namespace slog
