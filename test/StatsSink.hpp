#pragma once
#include "LogSink.hpp"
#include <vector>

class StatsSink : public slog::LogSink
{
public:
    StatsSink(long capacity) { sample.reserve(capacity); }
    ~StatsSink();
    void record(slog::LogRecord const& rec) override;
protected:
    std::vector<long> sample;
};
