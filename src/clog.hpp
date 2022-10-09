#pragma once
#include "LogCore.hpp"

#define Logf(level, ...) SLOG_LogBase(level, "", slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Logft(level, tag, ...) SLOG_LogBase(level, tag, slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Logftc(level, tag, channel, ...) SLOG_LogBase(level, tag, channel, __VA_ARGS__)

#define SLOG_LogBase(level, tag, channel, format, ...) \
        if (SLOG_LOGGING_ENABLED && slog::detail::should_log(level, tag, channel)) {  \
            LogRecord* rec = slog::detail::allocate_record(channel); \
            if (rec) { \
                slog::detail::take_record(rec->capture(__FILE__, __FUNCTION__, __LINE__, level, \
                                          channel, tag, format, ##__VA_ARGS__)); \
            } \
        }
