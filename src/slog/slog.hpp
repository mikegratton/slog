#pragma once
#include "config.hpp"
#include "slogDetail.hpp"

#if SLOG_STREAM_LOG
/**
 * This defines a set of macros that work like
 *
 * ```
 * Slog(INFO) << "Log with no tag to the default channel at level INFO";
 * Slog(WARN, "foo") << "Log with foo tag to the default channel at level WARN";
 * Slog(DBUG, "", 1) << "Log with no tag to channel 1 at level DBUG";
 * ```
 *
 * The macros are built such that if logging is suppressed for the combo of
 * severity level/tag/channel, then the rest of the line IS NOT EXECUTED. Any
 * formatting implied is just skipped. The log stream is derived from
 * std::ostream, but writes to a fixed-size internal buffer. If the record is
 * longer than one node can hold, extra nodes are allocated automatically and
 * chained together to form a "jumbo" record.
 *
 */
// Overloads for Slog() used below
#define SLOG_Logs(severity) SLOG_LogStreamBase(slog::severity, "", slog::DEFAULT_CHANNEL)
#define SLOG_Logst(severity, tag) SLOG_LogStreamBase(slog::severity, (tag), slog::DEFAULT_CHANNEL)
#define SLOG_Logstc(severity, tag, channel) SLOG_LogStreamBase(slog::severity, (tag), (channel))

// Overload Slog() based on the argument count
#define Slog(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Logstc, SLOG_Logst, SLOG_Logs)(__VA_ARGS__)

#endif

#if SLOG_FORMAT_LOG
/**
 * std::format-style macros. These take format strings and variable argument
 * lists like
 * ```
 * Flog(INFO, "The answer is {}", 42);
 * ```
 * TODO Make a Flog object so these read Flog(severity, tag)("Hello {}", "world")
 */
#define SLOG_Flog(severity) SLOG_FlogBase(slog::severity, "", slog::DEFAULT_CHANNEL)
#define SLOG_Flogt(severity, tag) SLOG_FlogBase(slog::severity, (tag), slog::DEFAULT_CHANNEL)
#define SLOG_Flogtc(severity, tag, channel) SLOG_FlogBase(slog::severity, (tag), (channel))

// Overload Flog() based on the argument count
#define Flog(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Flogtc, SLOG_Flogt, SLOG_Flog)(__VA_ARGS__)
#endif

#if SLOG_BINARY_LOG
/**
 * This defines a set of macros that work like
 *
 * ```
 * Blog(INFO)(my_bytes, my_byte_count);
 * Blog(WARN, "foo")(my_other_bytes, count).record(even_more_bytes, count2);
 * Blog(DBUG, "", 1)(son_of_more_bytes, count3);
 * ```
 *
 * These macros never trigger allocating memory.
 *
 * The macros are built such that if logging is suppressed for the combo of
 * severity level/tag/channel, then the record() calls ARE NOT EXECUTED. If the
 * record is longer than one node can hold, extra nodes are allocated
 * automatically and chained together to form a "jumbo" record.
 *
 * @note The default binary sink formatter presents the tag as a way to
 * distinguish the binary blob when parsing the log file. If you are logging
 * without a tag, you should ensure you have a mechanism to determine the
 * message format.
 */
#define SLOG_Blogs(severity) SLOG_BlogBase(slog::severity, "", slog::DEFAULT_CHANNEL)
#define SLOG_Blogst(severity, tag) SLOG_BlogBase(slog::severity, (tag), slog::DEFAULT_CHANNEL)
#define SLOG_Blosgstc(severity, tag, channel) SLOG_BlogBase(slog::severity, (tag), (channel))
#define Blog(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Blosgstc, SLOG_Blogst, SLOG_Blogs)(__VA_ARGS__)
#endif

namespace slog
{
/**
 * @brief Basic startup function.  For configuration options, see LogSetup.hpp
 */
void start_logger(int severity = INFO);

/**
 * @brief Stop all channels, draining the queue into the sinks.
 *
 * Note: this prevents further messages from being logged.
 */
void stop_logger();

/**
 * @brief Check if the current setup will log at the given severity (and optional tag and channel).
 * This can be used to avoid building strings that are only intended for logging.
 */
bool will_log(int severity, char const* tag = "", int channel = DEFAULT_CHANNEL);

/**
 * @brief Obtain the number of free record in the pool for the given channel.
 *
 * This can be used to tune the pool parameters. Using an ALLOCATE policy with a
 * small number of records per allocation, run the program periodically logging
 * free_record_count(). From this, you can determine an acceptable pool size to
 * use with the BLOCK policy.
 */
long free_record_count(int channel = DEFAULT_CHANNEL);

} // namespace slog

/**
 * @brief Async-signal-safe signal handler
 *
 * If you are handling a signal and want Slog to stop, this is the function to
 * call. It will block until the log queues are drained.
 */
extern "C" void slog_handle_signal(int signal_id);
