#include "LoggerSingleton.hpp"
#include "FileSink.hpp"
#include <cstdarg>

namespace slog {

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
    Logger::setup_stopped_channel();    
}

bool will_log(int severity, char const* tag, int channel)
{
    return severity <= Logger::get_channel(channel).threshold(tag);
}

void push_to_sink(RecordNode* node, int channel)
{
    if (node) { Logger::get_channel(channel).push(node); }
}

RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line, int severity,
                             char const* tag)
{
    RecordNode* node = Logger::get_channel(channel).get_fresh_record();
    if (node) { node->rec.meta.capture(file, function, line, severity, tag); }
    return node;
}

/// printf-style record capture
RecordNode* capture_message(RecordNode* node, char const* format_, ...)
{
    if (node) {
        va_list vlist;
        va_start(vlist, format_);
        node->rec.message_byte_count = vsnprintf(node->rec.message, node->rec.message_max_size, format_, vlist) + 1;
        va_end(vlist);
    }
    return node;
}

}