#include "testUtilities.hpp"
#include "LogConfig.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <ctime>

namespace slog
{

void rmdir(char const* dirname) { ::rmdir(dirname); }

int severity_from_string(char const* text)
{
    if (strncmp(text, "DBUG", 4) == 0) {
        return slog::DBUG;
    }
    if (strncmp(text, "INFO", 4) == 0) {
        return slog::INFO;
    }
    if (strncmp(text, "NOTE", 4) == 0) {
        return slog::NOTE;
    }
    if (strncmp(text, "WARN", 4) == 0) {
        return slog::WARN;
    }
    if (strncmp(text, "ERRR", 4) == 0) {
        return slog::ERRR;
    }
    if (strncmp(text, "CRIT", 4) == 0) {
        return slog::CRIT;
    }
    if (strncmp(text, "ALRT", 4) == 0) {
        return slog::ALRT;
    }
    if (strncmp(text, "EMER", 4) == 0) {
        return slog::EMER;
    }
    return slog::FATL;
}

bool parse_time(uint64_t& o_nanoseconds, char const* time_str)
{
    // 2020-01-02 12:13:14.567Z    
    constexpr uint64_t NANOS_PER_SEC   = 1000000000ULL;
    constexpr uint64_t NANOS_PER_MILLI = 1000000ULL;
    
    struct tm time_count{};    
    time_count.tm_year = atoi(time_str) - 1900;
    time_str += 5;
    time_count.tm_mon = atoi(time_str) - 1;
    time_str += 3;
    time_count.tm_mday = atoi(time_str);
    time_str += 3;
    time_count.tm_hour = atoi(time_str);
    time_str += 3;
    time_count.tm_min = atoi(time_str);
    time_str += 3;
    double seconds = atof(time_str);
    time_count.tm_sec = (int) std::trunc(seconds);
    uint64_t millis = 1000 * (seconds - std::trunc(seconds));
    time_t time = timegm(&time_count);
    o_nanoseconds = time * NANOS_PER_SEC + millis * NANOS_PER_MILLI;
    return true;
}

void parseLogRecord(TestLogRecord& o_record, char const* recordString)
{        
    int severity = 0; 
    char tag[TAG_SIZE]{}; 
    uint64_t time = 0;
    
    assert(recordString[0] == '[');
    recordString++;
    severity = severity_from_string(recordString);    
    recordString += 5;
    if (recordString[23] != ']') { // There's a tag
        char* cursor = tag;
        while (*recordString != ' ' && *recordString) {
            *cursor++ = *recordString++;
        }
        *cursor++ = '\0';
        recordString++;
    }
    parse_time(time, recordString);
    recordString += 23;
    assert(recordString[0] == ']');    
    o_record.meta.set_data("", "", 1, severity, tag, time, -1L);
    strncpy(o_record.message, recordString + 2, sizeof(o_record.message));
    o_record.message_byte_count = strlen(o_record.message) - 1; // exclude newline
}

} // namespace slog