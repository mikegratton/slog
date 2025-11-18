#include "FileSink.hpp"
#include "LoggerSingleton.hpp"
#include "RecordInserter.hpp"
#include <cstdarg>
#include <cassert>
#include "Locale.hpp"

namespace slog
{

using Logger = ::slog::detail::Logger;

void start_logger(int severity)
{
    LogConfig config;
    config.set_default_threshold(severity);
    config.set_sink(std::make_shared<FileSink>());
    start_logger(config);
}

void stop_logger()
{
    Logger::stop_all_channels();
}

bool will_log(int severity, char const* tag, int channel)
{
    return severity <= Logger::get_channel(channel).threshold(tag);
}

long free_record_count(int channel)
{
    return Logger::get_channel(channel).pool_free_count();
}

void push_to_sink(LogRecord* node, int channel) { Logger::get_channel(channel).push(node); }

LogRecord* get_fresh_record(int channel, char const* file, char const* function, int line, int severity,
                            char const* tag)
{
    LogRecord* node = Logger::get_channel(channel).get_fresh_record();
    if (node) {
        node->meta().capture(file, function, line, severity, tag);
    }
    return node;
}

LogRecord* capture_message(LogRecord* node, char const* format, ...)
{
    if (node) {
        va_list vlist;
        va_start(vlist, format);
        uint32_t bytes_written = vsnprintf(node->message(), node->capacity(), format, vlist);
        // Note: we don't record the null terminator. If the record was truncated, just indicate 
        // it is full
        node->size( std::min(bytes_written, node->capacity()) );
        va_end(vlist);
    }
    return node;
}

} // namespace slog


#if SLOG_FORMAT_LOG
#include <format>
namespace slog {

/**
 * @brief std::format log capture
 */
void format_log(LogRecord* rec, int channel, std::string_view format, std::format_args args)
{
    assert(rec);
    RecordInserter inserter(rec, channel);
    std::vformat_to<RecordInserterIterator>(RecordInserterIterator(&inserter), get_locale(), format, args);
}
} // namespace slog
#endif
