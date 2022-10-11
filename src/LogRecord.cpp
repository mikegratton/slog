#include "LogRecord.hpp"
#include <chrono>
#include <thread>
#include <cstring>

namespace slog {

LogRecordMetadata::LogRecordMetadata() {
    reset();
}

void LogRecordMetadata::reset() {
    filename = "";
    function = "";
    line = NO_LINE;
    severity = 2 >> 31;
    channel = DEFAULT_CHANNEL;
    tag[0] = '\0';
}

void LogRecordMetadata::capture(char const* filename_, char const* function_, int line_,
                                int severity_, int channel_, const char* tag_) {
    filename = filename_;
    function = function_;
    line = line_;
    time = std::chrono::system_clock::now().time_since_epoch().count();
    severity = severity_;
    channel = channel_;
    thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
    if (tag_) {
        strncpy(tag, tag_, sizeof(tag));
    }    
}

LogRecord::LogRecord(long max_message_size_) {
    message_max_size = max_message_size_;
    message = reinterpret_cast<char*>(this) + sizeof(LogRecord);
    reset();
}

void LogRecord::reset() {
    next = nullptr;
    meta.reset();
    message[0] = '\0';
}

}
