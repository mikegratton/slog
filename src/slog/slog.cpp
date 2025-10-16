#include "LoggerSingleton.hpp"

namespace slog {

using Logger = ::slog::detail::Logger;

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

}