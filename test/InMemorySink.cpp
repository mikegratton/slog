#include "InMemorySink.hpp"
#include "slog/LogRecord.hpp"

void InMemorySink::record(slog::LogRecord const& rec)
{
    mcontents.emplace_back();
    std::string& next_record = mcontents.back();

    for (slog::LogRecord const* cursor = &rec; cursor != nullptr; cursor = cursor->more()) {
        next_record.insert(next_record.end(), rec.message(), rec.message() + rec.size());        
    }
}
