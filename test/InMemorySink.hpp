#pragma once
#include "slog/LogSink.hpp"
#include <vector>
#include <string>

/// Log to an in-memory buffer for unit tests
class InMemorySink : public slog::LogSink {
   public:

    /// Try to get the mutex, then write
    void record(slog::LogRecord const& rec) override;

    std::vector<std::string> const& contents() const { return mcontents; }

   protected:
    std::vector<std::string> mcontents;
};
