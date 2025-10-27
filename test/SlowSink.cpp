#include "SlowSink.hpp"
#include <mutex>

SlowSink::SlowSink() { mwait_for_end.lock(); }

SlowSink::~SlowSink() {}

void SlowSink::finalize()
{    
    std::lock_guard<std::mutex> guard(mwait_for_end);
    printf("SlowSink::finalize(). Have lock, content count = %ld\n", mcontents.size());
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
    printf("%s\n", rec.message);
    std::lock_guard<std::mutex> guard(mwait_for_end);
    mcontents.push_back(rec.message);    
}