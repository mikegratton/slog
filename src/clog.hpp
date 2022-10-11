#pragma once
#include "LogCore.hpp"
#include "CapturePrintf.hpp"

/**
 * printf logging interface.
 * 
 * This defines a set of macros that work like
 * 
 * ```
 * Logf(INFO, "Log with no tag to the default channel at level %s", "INFO");
 * Logft(WARN, "foo", "Log with %s tag to the default channel at level %s", "foo", "WARN");
 * Logftc(DBUG, "", 1, "Log with no tag to channel %d at level %s", 1, "DBUG");
 * ```
 * 
 * These macros never trigger allocating memory.
 * 
 * The macros are built such that if logging is suppressed for the combo of severity
 * level/tag/channel, then the rest of the line IS NOT EXECUTED. Any formatting implied
 * is just skipped. Internally, snprintf() is used to write to a fixed-size internal buffer. 
 * If the message is longer than the buffer, the overrun is simply discarded.
 * 
 */

#define Logf(severity, ...) SLOG_LogBase(severity, "", slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Logft(severity, tag, ...) SLOG_LogBase(severity, tag, slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Logftc(severity, tag, channel, ...) SLOG_LogBase(severity, tag, channel, __VA_ARGS__)

#define SLOG_LogBase(severity, tag, channel, format, ...) \
        if (SLOG_LOGGING_ENABLED && slog::will_log(severity, tag, channel)) {  \
            LogRecord* rec = slog::get_fresh_record(channel, __FILE__, __FUNCTION__, \
                                                           __LINE__, severity, tag); \
            if (rec) { \
                slog::push_to_sink(slog::capture_message(rec, format, ##__VA_ARGS__)); \
            } \
        }
