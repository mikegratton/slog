#pragma once
#include "LogConfig.hpp"
#include "slogDetail.hpp"

/**
 * This defines a set of macros that work like
 *
 * ```
 * Slog(INFO) << "Log with no tag to the default channel at level INFO";
 * Slog(WARN, "foo") << "Log with foo tag to the default channel at level WARN";
 * Slog(DBUG, "", 1) << "Log with no tag to channel 1 at level DBUG";
 * ```
 *
 * These macros never trigger allocating memory.
 *
 * The macros are built such that if logging is suppressed for the combo of severity
 * level/tag/channel, then the rest of the line IS NOT EXECUTED. Any formatting implied
 * is just skipped. The log stream is derived from std::ostream, but writes to a fixed-
 * size internal buffer.
 *
 */

#if SLOG_STREAM

// Overloads for Slog() used below
#define SLOG_Logs(severity) SLOG_LogStreamBase(slog::severity, "", slog::DEFAULT_CHANNEL)
#define SLOG_Logst(severity, tag) SLOG_LogStreamBase(slog::severity, (tag), slog::DEFAULT_CHANNEL)
#define SLOG_Logstc(severity, tag, channel) SLOG_LogStreamBase(slog::severity, (tag), (channel))

// Overload Slog() based on the argument count
#define Slog(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Logstc, SLOG_Logst, SLOG_Logs)(__VA_ARGS__)

#endif

/**
 * printf-style macros. These take printf format string and variable argument lists like
 * ```
 * Flog(INFO, "The answer is %d", 42);
 * ```
 * Unlike Slog() and Blog(), Flog() macros can only log up to max_message_size_ bytes.
 */
#define Flog(severity, ...) SLOG_FlogBase(slog::severity, "", slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Flogt(severity, tag, ...) SLOG_FlogBase(slog::severity, (tag), slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Flogtc(severity, tag, channel, ...) SLOG_FlogBase(slog::severity, (tag), (channel), __VA_ARGS__)

#if SLOG_BINARY_LOG
/**
 * This defines a set of macros that work like
 *
 * ```
 * Blog(INFO).record(my_bytes, my_byte_count);
 * Blog(WARN, "foo").record(my_other_bytes, count).record(even_more_bytes, count2);
 * Blog(DBUG, "", 1).record(son_of_more_bytes, count3);
 * ```
 *
 * These macros never trigger allocating memory.
 *
 * The macros are built such that if logging is suppressed for the combo of severity
 * level/tag/channel, then the record() calls ARE NOT EXECUTED.
 *
 */
#define SLOG_Blogs(severity) SLOG_BlogBase(slog::severity, "", slog::DEFAULT_CHANNEL)
#define SLOG_Blogst(severity, tag) SLOG_BlogBase(slog::severity, (tag), slog::DEFAULT_CHANNEL)
#define SLOG_Blosgstc(severity, tag, channel) SLOG_BlogBase(slog::severity, (tag), (channel))
#define Blog(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Blosgstc, SLOG_Blogst, SLOG_Blogs)(__VA_ARGS__)
#endif

namespace slog {
/**
 * @brief Basic startup function.  For configuration options, see LogSetup.hpp
 */
void start_logger(int severity = INFO);

/**
 * @brief Check if the current setup will log at the given severity (and optional tag and channel).
 * This can be used to avoid building strings that are only intended for logging.
 */
bool will_log(int severity, char const* tag = "", int channel = DEFAULT_CHANNEL);

}  // namespace slog
