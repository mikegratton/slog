#pragma once
#include <cstdint>
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
    FileSink(FileSink const&) = delete;
    FileSink(FileSink&&) noexcept = default;
    FileSink& operator=(FileSink const&) = delete;
    FileSink& operator=(FileSink&&) noexcept= default;

    /**
     * @brief Close the open file.
     */
    void finalize() override;

    /**
     * @brief The header is inserted into each log file before any records.
     */
    void set_file_header_format(LogFileFurniture headerFormat);

    /**
     * @brief The footer is written to the log file after all records.
     */
    void set_file_footer_format(LogFileFurniture footerFormat);

    /// Write the record to the file
    virtual void record(LogRecord const& node) override;

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

    /// Get the name of the last log file opened
    char const* get_file_name() const { return mfullLogName; }

    /// Get the timestamp string applied to each file. This is the nanosecond
    /// count from the unix epoch.
    uint64_t get_start_timestamp() const { return msessionStartTime; }

   protected:

    void open_or_rotate();

    void close_file();

    void make_file_name();

    FILE* mfile;
    Formatter mformat;
    LogFileFurniture mheader;
    LogFileFurniture mfooter;
    char mlogDirectory[1024];
    char mlogBaseName[256];
    char mlogExtension[128];
    char msessionStartTimeStr[20];
    uint64_t msessionStartTime;
    int msequence;
    long mbytesWritten;
    long mmaxBytes;
    bool mecho;

    char mfullLogName[sizeof(mlogDirectory) + sizeof(mlogBaseName) + sizeof(mlogExtension) + sizeof(msessionStartTimeStr) + 8];
};

}  // namespace slog
