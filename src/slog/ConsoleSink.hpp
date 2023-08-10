#pragma once
#include "LogSink.hpp"

namespace slog {

/// Simple sink that dumps messages to stdout
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

}  // namespace slog
