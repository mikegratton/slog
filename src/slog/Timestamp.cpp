#include "Timestamp.hpp"
#include "PlatformUtilities.hpp"
#include <chrono>
#include <cstdint>

namespace slog
{

Timestamp Timestamp::now() { return Timestamp{(uint64_t)std::chrono::system_clock::now().time_since_epoch().count()}; }

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

void Timestamp::format_time(char* time_str, int seconds_precision, TimeFormatMode format) const
{
    constexpr uint64_t NANOS_PER_SEC = 1000000000ULL;
    time_t seconds_since_epoch = m_nanosSinceEpoch / NANOS_PER_SEC;
    uint64_t nano_remainder = m_nanosSinceEpoch - seconds_since_epoch * NANOS_PER_SEC;
    tm time_count;
    ::slog::zulu_time_r(&time_count, &seconds_since_epoch);
    char* cursor = time_str;

    cursor = put_int(cursor, 1900 + time_count.tm_year, 1000, 4);
    if (format != COMPACT) {
        *cursor++ = '-';
    }
    cursor = put_int(cursor, 1 + time_count.tm_mon, 10, 2);
    if (format != COMPACT) {
        *cursor++ = '-';
    }
    cursor = put_int(cursor, time_count.tm_mday, 10, 2);
    *cursor++ = (format == FULL_SPACE ? ' ' : 'T');
    cursor = put_int(cursor, time_count.tm_hour, 10, 2);
    if (format != COMPACT) {
        *cursor++ = ':';
    }
    cursor = put_int(cursor, time_count.tm_min, 10, 2);
    if (format != COMPACT) {
        *cursor++ = ':';
    }
    cursor = put_int(cursor, time_count.tm_sec, 10, 2);
    if (seconds_precision > 0) {
        *cursor++ = '.';
        cursor = put_int(cursor, nano_remainder, 100000000, seconds_precision);
    }
    *cursor++ = 'Z';
    *cursor = 0;
}

} // namespace slog