#pragma once
#include "LogRecord.hpp"

namespace slog {
namespace detail
{
bool should_log(int level, char const* tag="", int channel=DEFAULT_CHANNEL);

void take_record(LogRecord* rec);

LogRecord* allocate_record(int channel=DEFAULT_CHANNEL);

}
}

#define SLOG_LogBase(level, tag, channel, format, ...) \
    {\
        if (SLOG_LOGGING_ENABLED && slog::detail::should_log(level, tag, channel)) {  \
            LogRecord* rec = slog::detail::allocate_record(channel); \
            if (rec) { \
                slog::detail::take_record(rec->capture(__FILE__, __FUNCTION__, __LINE__, level, \
                                          channel, tag, format, ##__VA_ARGS__)); \
            } \
        }\
    }
