#include "slowSink.hpp"
#include "slog/LogSink.hpp"

SlowSink::SlowSink()
{
    mwait_for_end.lock();
}

SlowSink::~SlowSink()
{    
    
}

void SlowSink::record(slog::LogRecord const& rec)
{
    mwait_for_end.lock();
    slog::default_format(stdout, rec);
    fputc('\n', stdout);
    fflush(stdout);
    mwait_for_end.unlock();
}