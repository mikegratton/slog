#pragma once
#include <cstdio>

#include "LogSink.hpp"

namespace slog {

/**
 * @brief Simple file storage for log records
 * FileSink writes records to conventional files. It supports
 * - optional echo to the console
 * - File rotation
 * - Custom file naming
 * - Custom record formatting
 */
class FileSink : public LogSink {
   public:
    FileSink();
    ~FileSink();

    /// Write the record to the file
    void record(LogRecord const& node) final;

    /// Change the formatting for each record
    void set_formatter(Formatter format) { mformat = format; }

    /// Set echo to console on/off
    void set_echo(bool doit = true) { mecho = doit; }

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

   protected:
    void open_or_rotate();

    FILE* mfile;
    Formatter mformat;
    char mfileLocation[256];
    char mfileName[128];
    char mfileEnd[128];
    char msessionStartTime[20];
    int msequence;
    long mbytesWritten;
    long mmaxBytes;
    bool mecho;
};

}  // namespace slog
