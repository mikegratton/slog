#pragma once
#include <vector>

#include "slog/LogSink.hpp"

class StatsSink : public slog::LogSink {
   public:
    StatsSink(long capacity) { sample.reserve(capacity); }
    ~StatsSink();
    void record(slog::LogRecord const& rec) override;

   protected:
    std::vector<long> sample;
};
