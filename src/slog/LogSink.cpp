#include "LogSink.hpp"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "ConsoleSink.hpp"

namespace slog {

int default_format(FILE* sink, LogRecord const& rec)
{
    char time_str[32];
    format_time(time_str, rec.meta.time, 3, FULL_T);
    int writeCount = 0;
    fputc('[', sink);
    writeCount += fwrite(severity_string(rec.meta.severity), sizeof(char), 4, sink);
    fputc(' ', sink);
    writeCount += 2;
    if (rec.meta.tag[0]) {
        writeCount += fwrite(rec.meta.tag, sizeof(char), strnlen(rec.meta.tag, sizeof(rec.meta.tag)), sink);
        fputc(' ', sink);
        writeCount += 1;
    }
    writeCount += fwrite(time_str, sizeof(char), strnlen(time_str, sizeof(time_str)), sink);
    writeCount += fwrite("] ", sizeof(char), 2, sink);
    writeCount += no_meta_format(sink, rec);
    return writeCount;
}

int no_meta_format(FILE* sink, LogRecord const& rec)
{
    int count = 0;
    count += fwrite(rec.message, sizeof(char), strnlen(rec.message, rec.message_max_size), sink);
    for (LogRecord const* more = rec.more; more != nullptr; more = more->more) {
        count += fwrite(more->message, sizeof(char), strnlen(more->message, more->message_max_size), sink);
    }
    return count;
}

char const* severity_string(int severity)
{
    if (severity >= DBUG) {
        return "DBUG";
    } else if (severity >= INFO) {
        return "INFO";
    } else if (severity >= NOTE) {
        return "NOTE";
    } else if (severity >= WARN) {
        return "WARN";
    } else if (severity >= ERRR) {
        return "ERRR";
    } else if (severity >= CRIT) {
        return "CRIT";
    } else if (severity >= ALRT) {
        return "ALRT";
    } else if (severity >= EMER) {
        return "EMER";
    } else {
        return "FATL";
    }
}

void format_severity(char* severity_str, int severity)
{
    char const* severity_const_string = severity_string(severity);
    strncpy(severity_str, severity_const_string, 5);
}

/*
 * Low-level int to string function. Print value with the given number of digits,
 * including leading zeros.
 *
 * cursor -- a char buffer with at least digits+1 space
 * value -- integer to format
 * scale -- defined so that 0 <= value/scale <= 9
 * digits -- log10(scale) + 1
 */
static char* put_int(char* cursor, long value, long scale, int digits)
{
    for (int i = 0; i < digits; i++) {
        long digit = value / scale;
        value -= digit * scale;
        scale /= 10;
        *cursor++ = static_cast<char>(digit) + '0';
    }
    *cursor = 0;
    return cursor;
}

void format_time(char* time_str, unsigned long nanoseconds, int seconds_precision, TimeFormatMode format)
{
    constexpr unsigned long NANOS_PER_SEC = 1000000000ULL;
    if (seconds_precision < 0) {
        seconds_precision = 0;
    } else if (seconds_precision > 9) {
        seconds_precision = 9;
    }

    time_t seconds_since_epoch = nanoseconds / NANOS_PER_SEC;
    unsigned long nano_remainder = nanoseconds - seconds_since_epoch * NANOS_PER_SEC;

    tm time_count;
    gmtime_r(&seconds_since_epoch, &time_count);
    char* cursor = time_str;

    cursor = put_int(cursor, 1900 + time_count.tm_year, 1000, 4);
    if (format != COMPACT) { *cursor++ = '-'; }
    cursor = put_int(cursor, 1 + time_count.tm_mon, 10, 2);
    if (format != COMPACT) { *cursor++ = '-'; }
    cursor = put_int(cursor, time_count.tm_mday, 10, 2);
    *cursor++ = (format == FULL_SPACE ? ' ' : 'T');
    cursor = put_int(cursor, time_count.tm_hour, 10, 2);
    if (format != COMPACT) { *cursor++ = ':'; }
    cursor = put_int(cursor, time_count.tm_min, 10, 2);
    if (format != COMPACT) { *cursor++ = ':'; }
    cursor = put_int(cursor, time_count.tm_sec, 10, 2);
    if (seconds_precision > 0) {
        *cursor++ = '.';
        cursor = put_int(cursor, nano_remainder, 100000000, seconds_precision);
    }
}

void format_location(char* location_str, char const* file_name, int line_number)
{
    snprintf(location_str, 63, "%s@%d", basename(file_name), line_number);
}

void ConsoleSink::record(LogRecord const& rec)
{
    mformat(stdout, rec);
    fputc('\n', stdout);
    fflush(stdout);
}

}  // namespace slog
