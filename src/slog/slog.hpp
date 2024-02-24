#pragma once
#include "LogConfig.hpp"

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
 * size internal buffer. If the message is longer than the buffer, the overrun is simply
 * discarded.
 *
 */

#if SLOG_STREAM
// Baseline logging macro. Wrap the log check in an if() body, and get a stream
// in the else clause (so that the << ... parts are on the else branch)
#define SLOG_LogStreamBase(severity, tag, channel)                                                                  \
    if (!(SLOG_LOGGING_ENABLED && slog::will_log((severity), (tag), (channel)))) {                                  \
    } else                                                                                                          \
        slog::CaptureStream(slog::get_fresh_record((channel), __FILE__, __FUNCTION__, __LINE__, (severity), (tag)), \
                            (channel))                                                                              \
            .stream()

// Specialize SLOG_LogStreamBase for different numbers of arguments
#define SLOG_Logs(severity) SLOG_LogStreamBase(slog::severity, "", slog::DEFAULT_CHANNEL)
#define SLOG_Logst(severity, tag) SLOG_LogStreamBase(slog::severity, (tag), slog::DEFAULT_CHANNEL)
#define SLOG_Logstc(severity, tag, channel) SLOG_LogStreamBase(slog::severity, (tag), (channel))

// Overload Slog() based on the argument count
#define SLOG_GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define Slog(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Logstc, SLOG_Logst, SLOG_Logs)(__VA_ARGS__)

#endif

/**
 * printf-style macros. These take printf format string and variable argument lists like
 * ```
 * Flog(INFO, "The answer is %d", 42);
 * ```
 */
#define Flog(severity, ...) SLOG_FlogBase(slog::severity, "", slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Flogt(severity, tag, ...) SLOG_FlogBase(slog::severity, (tag), slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Flogtc(severity, tag, channel, ...) SLOG_FlogBase(slog::severity, (tag), (channel), __VA_ARGS__)

#define SLOG_FlogBase(severity, tag, channel, format, ...)                                                        \
    if (SLOG_LOGGING_ENABLED && slog::will_log((severity), (tag), (channel))) {                                   \
        RecordNode* rec = slog::get_fresh_record((channel), __FILE__, __FUNCTION__, __LINE__, (severity), (tag)); \
        if (rec) { slog::push_to_sink(slog::capture_message(rec, format, ##__VA_ARGS__), channel); }              \
    }

namespace slog {

/**
 * @brief Basic startup function.  For options, include LogSetup.hpp
 */
void start_logger(int severity = INFO);

/**
 * @brief Check if the current setup will log at the given severity (and optional tag and channel).
 * This can be used to avoid building strings that are only intended for logging.
 */
bool will_log(int severity, char const* tag = "", int channel = DEFAULT_CHANNEL);

/*** Implementation details follow ***/

// Internal record of a message
struct RecordNode;

/**
 * Internal method to send a completed record to the back end for recording.
 */
void push_to_sink(RecordNode* rec, int channel);

/**
 * Internal method to obtain a record from the pool
 */
RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line, int severity,
                             char const* tag);

// For printf-style messages, this provides variable argumnents
RecordNode* capture_message(RecordNode* rec, char const* format, ...);
}  // namespace slog

#if SLOG_STREAM
#include <ostream>
namespace slog {
// A special ostream wrapper that writes to node's message buffer.
// On destruction, the node is pushed to the back end
class CaptureStream {
   public:
    CaptureStream(RecordNode* node_, int channel_) : node(node_), channel(channel_) {}

    ~CaptureStream();

    std::ostream& stream();

   protected:
    RecordNode* node;
    int channel;
};
}  // namespace slog
#endif
