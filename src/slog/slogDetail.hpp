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

// Baseline printf-logging macro. Only allocate, capture, and push if the
// tag/severity passes the threshold.
#define SLOG_FlogBase(severity, tag, channel, format, ...)                     \
  if (SLOG_LOGGING_ENABLED && slog::will_log((severity), (tag), (channel))) {  \
    slog::RecordNode *rec = slog::get_fresh_record(                            \
        (channel), __FILE__, __FUNCTION__, __LINE__, (severity), (tag));       \
    if (rec) {                                                                 \
      slog::push_to_sink(slog::capture_message(rec, format, ##__VA_ARGS__),    \
                         channel);                                             \
    }                                                                          \
  }

// Bseline binary logging macro. Only allocate and capture if the tag/severity
// passes the threshold.
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
 * @brief Send a completed record to the back end for recording.
 */
void push_to_sink(RecordNode* rec, int channel);

/**
 * @brief Obtain a record from the pool, setting the metadata
 */
RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line, int severity,
                             char const* tag);

/**
 * printf-style message capture.
 * @warning This will truncate messages longer than one RecordNode long.
 */
RecordNode* capture_message(RecordNode* rec, char const* format, ...);

/**
* @brief Capture binary messages. 
*
* If a message is longer than one node, additional nodes will be allocated
* from the pool to create a "jumbo" message.
*
* Calls to record() can be chained together:
* Blog(NOTE, "foo").record(some_bytes, some_bytes_size).record(more_bytes, more_bytes_size)
*/
class CaptureBinary {
   public:
    CaptureBinary(RecordNode* node_, int channel_);
    ~CaptureBinary();
    CaptureBinary(CaptureBinary const&) = delete;
    CaptureBinary& operator=(CaptureBinary const&) = delete;

    CaptureBinary& record(void const* bytes, long byte_count);

   private:
    long write_some(char const* bytes, long byte_count);
    void set_node(RecordNode* new_node);
    void set_byte_count();

    RecordNode* head_node;
    RecordNode* current_node;    
    char* cursor;
    char const* buffer_end;
    int channel;
};
}  // namespace slog

#if SLOG_STREAM
#include <ostream>
namespace slog {

/**
 * @brief  A special ostream wrapper that writes to node's message buffer. On
 * destruction, the node is pushed to the backend. 
 *
 * If a message is too long for one node, additional nodes will be allocated
 * and chained together into a "jumbo" message.
 */
class CaptureStream {
   public:
    CaptureStream(RecordNode* node_, int channel_);
    CaptureStream(CaptureStream const&) = delete;
    CaptureStream& operator=(CaptureStream const&) = delete;
    ~CaptureStream();

    std::ostream& stream() { return *stream_ptr; }

   protected:
    std::ostream* stream_ptr;    
};
}  // namespace slog
#endif