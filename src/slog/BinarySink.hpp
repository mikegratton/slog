#pragma once
#include "LogSink.hpp"

namespace slog {

/**
 * @brief Basic logging of binary data to files using the C api.
 *
 * The default format includes an 8 byte file header described in LogSink.hpp,
 * a 32 byte record header per message, and no file footer. These are all
 * configurable.
 *
 * LIMITATIONS: This sink records severity and size as four byte ints, limiting
 * the maximum record size to 4GB and the maximum severity to 2^31 - 1
 */
class BinarySink : public LogSink {
   public:
    BinarySink();
    ~BinarySink();

    /**
     * @brief The header is inserted into each log file before any records.
     */
    void set_file_header_format(LogFileFurniture headerFormat);

    /**
     * @brief The footer is written to the log file after all records.
     */
    void set_file_footer_format(LogFileFurniture footerFormat);

    /**
     * @brief Change the formatting for each record
     */
    void set_formatter(Formatter format) { mformat = format; }

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

    LogFileFurniture mheader;
    LogFileFurniture mfooter;
    Formatter mformat;
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