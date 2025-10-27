#pragma once
#include "LogConfig.hpp"

#define SLOG_GET_MACRO(_1, _2, _3, NAME, ...) NAME

#if SLOG_STREAM
// Baseline logging macro. Wrap the log check in an if() body, and get a stream
// in the else clause (so that the << ... parts are on the else branch)
#define SLOG_LogStreamBase(severity, tag, channel)                                                                  \
    if (!(SLOG_LOGGING_ENABLED && slog::will_log((severity), (tag), (channel)))) {                                  \
    } else                                                                                                          \
        slog::CaptureStream(slog::get_fresh_record((channel), __FILE__, __FUNCTION__, __LINE__, (severity), (tag)), \
                            (channel))                                                                              \
            .stream()

#endif

#define SLOG_FlogBase(severity, tag, channel, format, ...)                     \
  if (SLOG_LOGGING_ENABLED && slog::will_log((severity), (tag), (channel))) {  \
    slog::RecordNode *rec = slog::get_fresh_record(                            \
        (channel), __FILE__, __FUNCTION__, __LINE__, (severity), (tag));       \
    if (rec) {                                                                 \
      slog::push_to_sink(slog::capture_message(rec, format, ##__VA_ARGS__),    \
                         channel);                                             \
    }                                                                          \
  }

#define SLOG_BlogBase(severity, tag, channel)                                                                       \
    if (!(SLOG_LOGGING_ENABLED && slog::will_log((severity), (tag), (channel)))) {                                  \
    } else                                                                                                          \
        slog::CaptureBinary(slog::get_fresh_record((channel), __FILE__, __FUNCTION__, __LINE__, (severity), (tag)), \
                            (channel))

namespace slog {
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

// For binary capture
class CaptureBinary {
   public:
    CaptureBinary(RecordNode* node_, long channel_);
    ~CaptureBinary();
    CaptureBinary(CaptureBinary const&) = delete;
    CaptureBinary& operator=(CaptureBinary const&) = delete;

    CaptureBinary& record(void const* bytes, long count);

   private:
    long write_some(char const* bytes, long count);
    void set_node(RecordNode* new_node, long channel);

    RecordNode* node;
    long channel;
    char* cursor;
    char const* end;
    long* currentByteCount;
};
}  // namespace slog

#if SLOG_STREAM
#include <ostream>
namespace slog {
// A special ostream wrapper that writes to node's message buffer.
// On destruction, the node is pushed to the back end
class CaptureStream {
   public:
    CaptureStream(RecordNode* node_, int channel_) : node(node_), channel(channel_) {}
    CaptureStream(CaptureStream const&) = delete;
    CaptureStream& operator=(CaptureStream const&) = delete;
    ~CaptureStream();

    std::ostream& stream();

   protected:
    RecordNode* node;
    int channel;
};
}  // namespace slog
#endif