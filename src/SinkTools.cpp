#include "SinkTools.hpp"
#include <cstdio>
#include <ctime>
#include <cstring>

namespace slog {


int default_format(FILE* sink, LogRecord const* rec)
{
    int count = 0;
    char level_str[16];
    char time_str[32];
    format_level(level_str, rec->severity);
    format_time(time_str, rec->time);
    if (rec->tag) {
        count += fprintf(sink, "[%s %s %s] ", level_str, rec->tag, time_str);
    } else {
        count += fprintf(sink, "[%s %s] ", level_str, time_str);
    }
    count += fprintf(sink, "%s", rec->message);
    return count;
}
    
void format_level(char* level_str, int level)
{
    if (level >= DBUG) {
        strncpy(level_str, "DBUG", 5);
    } else if (level >= INFO) {
        strncpy(level_str, "INFO", 5);
    } else if (level >= NOTE) {
        strncpy(level_str, "NOTE", 5);
    } else if (level >= WARN) {
        strncpy(level_str, "WARN", 5);
    } else if (level >= ERRR) {
        strncpy(level_str, "ERRR", 5);
    } else if (level >= CRIT) {
        strncpy(level_str, "CRIT", 5);
    } else if (level >= ALRT) {
        strncpy(level_str, "ALER", 5);
    } else if (level >= EMER) {
        strncpy(level_str, "EMER", 5);
    } else {
        strncpy(level_str, "FATL", 5);
    }
}

void format_time(char* time_str, unsigned long t, int seconds_precision)
{
    constexpr unsigned long NANOS_PER_SEC = 1000000000ULL;
    const int min_size = 18;
    time_t seconds_since_epoch = t / NANOS_PER_SEC;
    unsigned long nano_remainder = t % NANOS_PER_SEC;
    
    tm broken;
    gmtime_r(&seconds_since_epoch , &broken);
    
    // Most of the iso timestamp
    auto offset = strftime(time_str, min_size, "%Y-%m-%d %H:%M:", &broken);
    
    // Seconds and zone
    double secondsPart = static_cast<double>(broken.tm_sec) + 1e-9 * nano_remainder;
    sprintf(time_str+offset, "%06.3fZ", secondsPart);
}

void format_location(char* location_str, char const* file_name, int line_number)
{
    snprintf(location_str, 64, "%s@%d", basename(file_name), line_number);
}

}
