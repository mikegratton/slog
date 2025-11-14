#pragma once
#include <cstdio>

#include "LogSink.hpp"
#include "slog/LogRecord.hpp"

namespace slog {

/**
 * @brief Log using the systemd journal.
 *
 * To use this, you must link to libsystemd, typically via "-lsystemd".
 *
 * See http://0pointer.de/blog/projects/journalctl.html for info on seeing
 * and handling these log messages
 *
 * The following properties are set on each record:
 * - CODE_FUNC The name of the function where the record originated
 * - CODE_FILE The source file name where the record originated
 * - CODE_LINE The file line number where  the record originated
 * - THREAD    The thread id for the thread where the record originated
 * - TIMESTAMP ISO8601 formatted time of record
 * - PRIORITY  Syslog-compatible log severity
 * - TAG       The tag supplied with the record
 *
 */
class JournaldSink : public LogSink {
   public:
    JournaldSink();
    ~JournaldSink();
    JournaldSink(JournaldSink const&) = delete;
    JournaldSink& operator=(JournaldSink const&) = delete;
    JournaldSink(JournaldSink&&) noexcept = default;
    JournaldSink& operator=(JournaldSink&&) noexcept = default;

    /// Write a record to the journal
    void record(LogRecord const& rec) final;

    /**
     * @brief Change the console echo log format
     *
     * This only applies to console echo. Structured logging is
     * used in the journal call
     */
    void set_formatter(Formatter format) { format = format; }

    /**
     * @brief Turn on echoing to the console
     */
    void set_echo(bool doit = true) { echo = doit; }

    /**
    * @brief Determine the maximum line length in journald. @note This has to open
    * two child processes to get this info.
    */
    static long line_max();

   private:
    /// Write the current buffer to the journal
    void write_to_journal(LogRecord const& rec) const;

    // Max record size we can submit to the journal
    long buffer_size;
    // Current buffer fill
    long buffer_used;
    // LINE_MAX buffer for journal
    char* message_buffer;
    // Time of the current record
    char misotime[32];
    // For echoing to the console, this formats the messages
    Formatter formatter;
    // Do we echo to the console.
    bool echo;
};

}  // namespace slog
