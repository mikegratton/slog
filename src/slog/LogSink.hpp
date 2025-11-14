#pragma once
#include <cstdint>
#include <cstdio>
#include <functional>

#include "LogRecord.hpp"

namespace slog
{

/**
 * @brief Base class for all log sinks.
 *
 * To implement a new sink, only the record() function is required.
 */
class LogSink
{
  public:
    virtual ~LogSink() = default;

    /// Save a record to a device.
    virtual void record(LogRecord const& node) = 0;

    /// Notification that logging is done (i.e. close up any files)
    virtual void finalize() {}
};

/**
 * @brief A sink that discards messages
 */
class NullSink : public LogSink
{
  public:
    void record(LogRecord const&) override {}
};

/**
 * @brief Standard form for message formatters.
 * This should write rec to sink and return the number of bytes
 * written.
 */
using Formatter = std::function<uint32_t(FILE* sink, LogRecord const& node)>;

/**
 * @brief Write data to a log file before/after records are added
 */
using LogFileFurniture = std::function<int(FILE* sink, int sequence, uint64_t time)>;

/**
 * @brief Writes the severity level as a four character string to severity_str
 */
void format_severity(char* severity_str, int severity);

/**
 * @brief Obtain a pointer to the null-terminated four character severity string
 */
char const* severity_string(int severity);

enum TimeFormatMode {
    FULL_SPACE, // YYYY-MM-DD hh:mm:ss.fZ
    COMPACT,    // YYYYMMDDThhmmss.fZ
    FULL_T      // YYYY-MM-DDThh:mm:ss.fZ
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
 */
void format_time(char* time_str, uint64_t time, int seconds_decimal_precision = 3, TimeFormatMode format = FULL_SPACE);

/**
 * @brief Makes a code location string.
 *
 * This removes the path from file_name, then appends the line number: "Foo.cpp@15"
 * @note location_str must be at least size 64. Locations are truncated after 63 characters.
 */
void format_location(char* location_str, char const* file_name, int line_number);

/**
 * @brief The default record format: "[SEVR TAG YYYY-MM-DD hh:mm:ss.sssZ]"
 *
 * SEVR is the four character severity code from format_severity(), TAG is the
 * optional tag (if no tag, then nothing is printed).
 */
uint32_t default_format(FILE* sink, LogRecord const& node);

/**
 * A Formatter that just prints the records
 */
uint32_t no_meta_format(FILE* sink, LogRecord const& node);

/**
 * @brief This writes data in a byte delimited format with a 32 byte record header.
 *
 * Each record becomes an entry of the form
 *    <size><severity><time><tag><record...>
 * where
 *    <size> is a four byte unsigned int
 *    <severity> is a four byte signed int giving the size of the record (this does
 *               not include the size of the record leader)
 *    <time> is an eight byte signed nanosecond count since the unix epoch
 *    <tag> is 16 one-byte characters. The final character is always '\0'
 *    <record> is the logged data
 */
uint32_t default_binary_format(FILE* sink, LogRecord const& node);

/**
 * @brief This writes data in a byte delimited format with an 8 byte record header
 *
 * Each record becomes an entry of the form
 *    <size><tag><record...>
 * where
 *    <size> is a four byte unsigned int
 *    <tag> is the first four bytes of the tag (short tags are padded out with nulls)
 *    <record> is the logged data
 */
uint32_t short_binary_format(FILE* sink, LogRecord const& node);

/**
 * @brief Compute the total number of bytes in a record (including any attached extra
 * record nodes)
 */
uint32_t total_record_size(LogRecord const& node);

/**
 * @brief Simple 8 byte file header of the form "SLOG<BOM><SS>" where the literal
 * ascii characters "SLOG" start the file, followed by the two byte byte-order-mark
 * 0xFEFF, followed by the two byte sequence number. The time is not included.
 */
uint32_t default_binary_header_furniture(FILE* sink, int sequence, uint64_t time);

/**
 * @brief A furniture function that writes nothing
 */
inline uint32_t no_op_furniture(FILE* sink, int sequence, uint64_t time) { return 0; }

} // namespace slog
