#include "SlowSink.hpp"
#include <mutex>
#include <cassert>

SlowSink::SlowSink() { mwait_for_end.lock(); }

SlowSink::~SlowSink() {}

void SlowSink::finalize()
{    
    std::lock_guard<std::mutex> guard(mwait_for_end);
    if (mcontents.empty()) { return; }
    
    FILE* f = fopen(file_name(), "w");
    for (auto const& item : mcontents) {
        fputs(item.c_str(), f);
        fputc('\n', f);
    }
    fclose(f);
    mcontents.clear();
}

void SlowSink::record(slog::LogRecord const& rec)
{
    std::lock_guard<std::mutex> guard(mwait_for_end);
    std::string message(rec.message(), rec.message() + rec.size());
    for (auto const* cursor = rec.more(); cursor != nullptr; cursor = cursor->more()) {
        std::string nextPart(cursor->message(), cursor->message() + cursor->size());
        message += nextPart;
    }
    mcontents.push_back(message);
}