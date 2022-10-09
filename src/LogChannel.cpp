#include "LogChannel.hpp"
#include <cassert>
#include <chrono>

namespace slog {

constexpr std::chrono::milliseconds WAIT{50};

LogRecordPool s_allocator;

LogChannel::~LogChannel() {
    stop();
}

void LogChannel::push(LogRecord* rec) { 
    int level = rec->severity;
    queue.push(rec);
    if (level <= FATL) {
        abort();
    }
}

void LogChannel::stop() {
    keepalive = false;
    if (workThread.joinable()) {
        workThread.join();
    }
    runMode = false;
}

void LogChannel::set_sink(std::unique_ptr<LogSink> sink_) {
    assert(!runMode);
    sink = std::move(sink_);
}

void LogChannel::set_threshold(ThresholdMap const& threshold_) {
    assert(!runMode);
    thresholdMap = threshold_;
}

void LogChannel::start() {
    stop();

    runMode = true;
    keepalive = true;
    workThread = std::thread([this]() {
        while (keepalive) {
            LogRecord* rec = queue.pop(WAIT);
            if (rec) {
                sink->record(rec);
                pool.deallocate(rec);
            }
        }
        LogRecord* head = queue.pop_all();
        while (head) {
            sink->record(head);
            LogRecord* cursor = head->next;
            pool.deallocate(head);
            head = cursor;
        }
    });
}

}
