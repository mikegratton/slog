#include "LogRecord.hpp"
#include <chrono>
#include <cstdarg>
#include <chrono>
#include <cstdio>
#include <thread>

namespace slog {

LogRecord::LogRecord(long max_message_size_) {
    message_max_size = max_message_size_;
    message = reinterpret_cast<char*>(this) + sizeof(LogRecord);
    reset();
}

void LogRecord::reset() {
    next = nullptr;
    filename = "";
    function = "";
    line = NO_LINE;
    severity = INFO;
    channel = DEFAULT_CHANNEL;
    tag = "";
    message[0] = '\0';
}


LogRecord* LogRecord::capture(char const* filename_, char const* function_, int line_,
                              int severity_, int channel_, const char* tag_, 
                              char const* format_, ...) {
    next = nullptr;
    va_list vlist;
    va_start(vlist, format_);
    vsnprintf(message, message_max_size, format_, vlist);
    va_end(vlist);

    filename = filename_;
    function = function_;
    line = line_;
    time = std::chrono::system_clock::now().time_since_epoch().count();
    severity = severity_;
    channel = channel_;
    thread_id = std::hash<std::thread::id> {}(std::this_thread::get_id());
    tag = tag_;
    return this;
}

}
