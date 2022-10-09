#pragma once
#include "LogCore.hpp"
#include <iosfwd>

namespace slog {

namespace detail {
std::ostream& get_stream(char* cstring, long maxLength);
std::ostream& get_null_stream();
void terminate();

}

class CaptureStream {
public:
    CaptureStream(LogRecord* rec_) : rec(rec_) { }

    ~CaptureStream() {
        if (rec) {
            detail::terminate();
            detail::take_record(rec);
        }
    }

    std::ostream& stream() {
        if (rec) {
            return detail::get_stream(rec->message, rec->message_max_size);
        } else {
            return detail::get_null_stream();
        }
    }

protected:
    LogRecord* rec;
};
}

#define SLOG_LogStreamBase(level, tag, channel) \
        if (!(SLOG_LOGGING_ENABLED && slog::detail::should_log(level, tag, channel))) { } \
        else slog::CaptureStream(slog::detail::allocate_record(channel)).stream()
