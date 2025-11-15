#include "FileSink.hpp"
#include "LoggerSingleton.hpp"
#include <cstdarg>

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
        uint32_t bytes_written = vsnprintf(node->message(), node->message_max_size(), format, vlist);
        // Note: we don't record the null terminator. If the record was truncated, just indicate 
        // it is full
        node->message_byte_count( std::min(bytes_written, node->message_max_size()) );
        va_end(vlist);
    }
    return node;
}

} // namespace slog