#include "LogRecord.hpp"
#include <chrono>
#include <thread>
#include <cstring>

namespace slog {

constexpr int NO_LINE = -1;

LogRecordMetadata::LogRecordMetadata() {
    reset();
}

void LogRecordMetadata::reset() {
    filename = "";
    function = "";
    line = NO_LINE;
    severity = 2 >> 31;
    tag[0] = '\0';
}

void LogRecordMetadata::capture(char const* filename_, char const* function_, int line_,
                                int severity_, const char* tag_) {
    filename = filename_;
    function = function_;
    line = line_;
    time = std::chrono::system_clock::now().time_since_epoch().count();
    severity = severity_;    
    thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
    if (tag_) {
        strncpy(tag, tag_, sizeof(tag));
    }    
}

LogRecord::LogRecord(char* message_, long max_message_size_) {
    message_max_size = max_message_size_;
    message = message_;
    reset();
}

void LogRecord::reset() {    
    meta.reset();
    message[0] = '\0';
}

}
