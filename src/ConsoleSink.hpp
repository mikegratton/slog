#pragma once
#include "LogSink.hpp"

namespace slog 
{
// Simple sink that dumps messages to the console
class ConsoleSink : public LogSink
{
public:
    ConsoleSink() : mformat(default_format) { }
    void record(LogRecord const& rec) override;
    void set_formatter(Formatter format) { mformat = format; }
protected:
    Formatter mformat;
};

}
