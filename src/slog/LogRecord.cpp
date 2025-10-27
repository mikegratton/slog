#include "LogRecord.hpp"

#include <chrono>
#include <cstring>
#include <thread>
#include <limits>

namespace slog {

constexpr int NO_LINE = -1;

LogRecordMetadata::LogRecordMetadata()
{
    reset();
}

void LogRecordMetadata::reset()
{
    filename = "";
    function = "";
    line = NO_LINE;
    severity = std::numeric_limits<int>::max();
    memset(tag, 0, sizeof(tag));
}

void LogRecordMetadata::capture(char const* filename_, char const* function_, int line_, int severity_,
                                const char* tag_)
{
    filename = filename_;
    function = function_;
    line = line_;
    time = std::chrono::system_clock::now().time_since_epoch().count();
    severity = severity_;
    thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    if (tag_) { strncpy(tag, tag_, sizeof(tag) - 1); }
}

LogRecord::LogRecord(char* message_, long max_message_size_)
{
    message_max_size = max_message_size_;
    message_byte_count = 0L;
    message = message_;
    reset();
}

void LogRecord::reset()
{
    meta.reset();
    more = nullptr;
    message[0] = '\0';
    message_byte_count = 0L;
}

}  // namespace slog
