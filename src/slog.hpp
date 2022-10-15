#pragma once
#include <iosfwd>
#include "LogRecord.hpp"

/**
 * ostream logging interface.
 * 
 * This defines a set of macros that work like
 * 
 * ```
 * Log(INFO) << "Log with no tag to the default channel at level INFO";
 * Log(WARN, "foo") << "Log with foo tag to the default channel at level WARN";
 * Log(DBUG, "", 1) << "Log with no tag to channel 1 at level DBUG";
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

// Baseline logging macro. Wrap the log check in an if() body, and get a stream
// in the else clause (so that the << ... parts are on the else branch)
#define SLOG_LogStreamBase(severity, tag, channel) \
        if (!(SLOG_LOGGING_ENABLED && slog::will_log(severity, tag, channel))) { } \
        else slog::CaptureStream(slog::get_fresh_record(channel, __FILE__, __FUNCTION__, \
                                                        __LINE__, severity, tag)).stream()

// Specialize SLOG_LogStreamBase for different numbers of arguments
#define SLOG_Logs(severity) SLOG_LogStreamBase(severity, "", slog::DEFAULT_CHANNEL)
#define SLOG_Logst(severity, tag) SLOG_LogStreamBase(severity, tag, slog::DEFAULT_CHANNEL)
#define SLOG_Logstc(severity, tag, channel) SLOG_LogStreamBase(severity, tag, channel)

// Overload Log() based on the argument count
#define SLOG_GET_MACRO(_1,_2,_3,NAME,...) NAME
#define Slog(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Logstc, SLOG_Logst, SLOG_Logs)(__VA_ARGS__)

#define Flog(severity, ...) SLOG_LogBase(severity, "", slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Flogt(severity, tag, ...) SLOG_LogBase(severity, tag, slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Flogtc(severity, tag, channel, ...) SLOG_LogBase(severity, tag, channel, __VA_ARGS__)

#define SLOG_LogBase(severity, tag, channel, format, ...) \
        if (SLOG_LOGGING_ENABLED && slog::will_log(severity, tag, channel)) {  \
            RecordNode* rec = slog::get_fresh_record(channel, __FILE__, __FUNCTION__, \
                                                           __LINE__, severity, tag); \
            if (rec) { \
                slog::push_to_sink(slog::capture_message(rec, format, ##__VA_ARGS__)); \
            } \
        }

        
                                                    
namespace slog {

struct RecordNode;

// A special ostream wrapper that writes to node's message buffer.
// On destruction, the node is pushed to the back end
class CaptureStream {
public:
    CaptureStream(RecordNode* node_) : node(node_) { }

    ~CaptureStream();

    std::ostream& stream();

protected:
    RecordNode* node;
};

RecordNode* capture_message(RecordNode* rec, char const* format, ...);

bool will_log(int severity, char const* tag="", int channel=DEFAULT_CHANNEL);

void push_to_sink(RecordNode* rec);

RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line, 
                           int severity, char const* tag);
    
}
