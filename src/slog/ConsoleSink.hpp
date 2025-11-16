#pragma once
#include "LogSink.hpp"

namespace slog {

/**
 * @brief A simple sink that writes messages to stdout. Messages are guarateed
 * to end in '\n'.
 */
class ConsoleSink : public LogSink {
   public:
    ConsoleSink() : mformat(default_format) {}

    /// Write a record to the console
    void record(LogRecord const& rec) override;

    /// Change the metadata formatting
    void set_formatter(Formatter format) { mformat = format; }

   protected:
    Formatter mformat;
};

inline void ConsoleSink::record(LogRecord const& rec)
{
    mformat(stdout, rec);
    fputc('\n', stdout);
    fflush(stdout);
}

}  // namespace slog
