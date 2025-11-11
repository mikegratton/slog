#include "LogRecord.hpp"

#include <chrono>
#include <cstring>
#include <limits>
#include <thread>

namespace slog
{

constexpr int NO_LINE = -1;

LogRecordMetadata::LogRecordMetadata() { reset(); }

void LogRecordMetadata::reset()
{
    filename = "";
    function = "";
    line = NO_LINE;
    severity = std::numeric_limits<int>::max();    
    memset(tag, 0, sizeof(tag));
}

void LogRecordMetadata::capture(char const* filename_, char const* function_, int line_, int severity_,
                                char const* tag_)
{
    // Because we might be attaching a record to another record to form a "jumbo" record, sometimes the
    // metadata isn't meaningful. We avoid system calls in these cases.
    filename = filename_;
    function = function_;
    line = line_;
    severity = severity_;
    if (line >= 0) {
        time = std::chrono::system_clock::now().time_since_epoch().count();
        thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    }    
    if (tag_) {
        strncpy(tag, tag_, sizeof(tag) - 1);
    }
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
    message_byte_count = 0L;
    message[0] = '\0';
    more = nullptr;
}

} // namespace slog
