#pragma once
#include "LogSink.hpp"

namespace slog {

/**
 * @brief Basic logging of binary data to files using the C api.
 *
 * This writes data in a byte delimited format. Each record becomes
 * an entry of the form
 *    <size><severity><time><tag><record...>
 * where
 *    <size> is a four byte unsigned int
 *    <severity> is a four byte signed int giving the size of the record (this does
 *               not include the size of the record leader)
 *    <time> is an eight byte signed nanosecond count since the unix epoch
 *    <tag> is 16 one-byte characters. The final character is always '\0'
 *    <record> is the logged data
 * Each record has a 32 byte "leader" with the record metadata in compact form.
 *
 * LIMITATIONS: This sink records severity and size as four byte ints, limiting
 * the maximum record size to 4GB and the maximum severity to 2^31 - 1
 */
class BinarySink : public LogSink {
   public:
    BinarySink();
    ~BinarySink();

    /// Write the record to the file
    void record(LogRecord const& node) final;

    /// Change the maximum filesize before rotation
    void set_max_file_size(long isize);

    /**
     * @brief Change the filename
     * @param location The directory to place log files in
     * @param name The "leading" part of the file name
     * @param end The extension for the filename
     *
     * The overall name is of the form <location>/<name>_<timestamp>_<sequence>.<end>
     * where <timestamp> is the ISO8601 timestamp and <sequence> is a three digit
     * sequence number.
     */
    void set_file(char const* location, char const* name, char const* end = "log");

   private:
    void open_or_rotate();

    FILE* mfile;
    char mfileLocation[256];
    char mfileName[128];
    char mfileEnd[128];
    char msessionStartTime[20];
    int msequence;
    long mbytesWritten;
    long mmaxBytes;
};
}  // namespace slog