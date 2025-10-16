#pragma once
#include "slog/LogSink.hpp"
#include <mutex>

/// Block messages until deconstruction
class SlowSink : public slog::LogSink {
   public:
    /// Lock the mutex
    SlowSink();

    /// Write all buffered output
    ~SlowSink();

    void unlock() { mwait_for_end.unlock(); }

    /// Try to get the mutex, then write
    void record(slog::LogRecord const& rec) override;

   protected:    
    std::mutex mwait_for_end;
};
