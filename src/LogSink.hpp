#pragma once
#include "LogRecord.hpp"
#include <functional>

namespace slog {

// Standard form for message formatters. This should
// write rec to sink and return the number of bytes
// written.
using Formatter = std::function<int (FILE* sink, LogRecord const* rec)>;


class LogSink
{
public:
    virtual ~LogSink() = default;
    virtual void record(LogRecord const* rec) = 0;
};

// Simple sink that ignores messages
class NullSink : public LogSink
{
public:
    void record(LogRecord const*) override { }
};
}
