#include "LogChannel.hpp"

namespace slog
{

LogChannel::LogChannel(std::shared_ptr<LogSink> sink_, ThresholdMap const& threshold_,
                       std::shared_ptr<LogRecordPool> pool_)
    : pool(pool_),
      threshold_map(threshold_),
      sink(sink_)
{
}

LogChannel::~LogChannel() { sink->finalize(); }


void LogChannel::send_to_sink(LogRecord* node)
{
    if (node) {
        sink->record(*node);
        pool->free(node);
    }
}

void LogChannel::finalize()
{
    sink->finalize();
}

} // namespace slog
