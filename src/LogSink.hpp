#pragma once
#include "LogRecord.hpp"

namespace slog {

class LogSink
{
public:
    virtual ~LogSink() = default;
    virtual void record(LogRecord const& rec) = 0;
};

// Simple sink that ignores messages
class NullSink : public LogSink
{
public:
    void record(LogRecord const&) override { }
};

// Simple sink that dumps messages to the console
class ConsoleSink : public LogSink
{
public:
    void record(LogRecord const& rec) override;
};
}
