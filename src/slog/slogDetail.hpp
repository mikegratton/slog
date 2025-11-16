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
    slog::LogRecord *rec = slog::get_fresh_record(                            \
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
struct LogRecord;

/**
 * @brief Send a completed record to the back end for recording.
 */
void push_to_sink(LogRecord* rec, int channel);

/**
 * @brief Obtain a record from the pool, setting the metadata
 */
LogRecord* get_fresh_record(int channel, char const* file, char const* function, int line, int severity,
                             char const* tag);

/**
 * printf-style message capture.
 * @warning This will truncate messages longer than one LogRecord long.
 */
LogRecord* capture_message(LogRecord* rec, char const* format, ...);

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
    
    /// Construct a capture object with the given head node destined for
    /// the given channel
    CaptureBinary(LogRecord* node, int channel);

    /// Push the record to the channel sink
    ~CaptureBinary();

    /// This is not copyable
    CaptureBinary(CaptureBinary const&) = delete;
    CaptureBinary& operator=(CaptureBinary const&) = delete;
    CaptureBinary(CaptureBinary&&) noexcept = default;
    CaptureBinary& operator=(CaptureBinary&&) noexcept = default;

    /**
     * @brief Write bytes into the record
     * @param bytes -- Byte array
     * @param byte_count -- Byte count to write from bytes
     */
    CaptureBinary& record(void const* bytes, long byte_count);

   private:

    /// Write bytes to the current node. This will not write beyond that node's remaining capacity
    long write_some(char const* bytes, long byte_count);

    /// Set the node. If there's a non-null head_node, this node is attached to the current node
    /// and the cursor/buffer_end are reset to point to its message region
    void set_node(LogRecord* new_node);

    /// Set the number of bytes written to current_node
    void set_byte_count();

    LogRecord* head_node;
    LogRecord* current_node;    
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
    /// Make a capture object that writes to node destined for channel_
    CaptureStream(LogRecord* node, int channel);

    /// These are not copyable
    CaptureStream(CaptureStream const&) = delete;
    CaptureStream& operator=(CaptureStream const&) = delete;
    CaptureStream(CaptureStream&&) noexcept = default;
    CaptureStream& operator=(CaptureStream&&) noexcept = default;

    /// Push the node to the channel sink
    ~CaptureStream();

    /// Obtain the actual stream object
    std::ostream& stream() { return *stream_ptr; }

   private:
    std::ostream* stream_ptr;    
};
}  // namespace slog
#endif