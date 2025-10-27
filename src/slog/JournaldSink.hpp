#pragma once
#include <cstdio>

#include "LogSink.hpp"

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
    JournaldSink() : mformat(default_format), mecho(false) {}
    ~JournaldSink() = default;

    /// Write a record to the journal
    void record(LogRecord const& rec) final;

    /**
     * @brief Change the console echo log format
     *
     * This only applies to console echo. Structured logging is
     * used in the journal call
     */
    void set_formatter(Formatter format) { mformat = format; }

    /// Turn on/off echo to console
    void set_echo(bool doit = true) { mecho = doit; }

   protected:
    Formatter mformat;
    bool mecho;
};

}  // namespace slog
