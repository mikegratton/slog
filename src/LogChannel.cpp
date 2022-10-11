#include "LogChannel.hpp"
#include <cassert>
#include <chrono>

namespace slog {

constexpr std::chrono::milliseconds WAIT{50};

LogRecordPool s_allocator(SLOG_LOG_POOL_SIZE, SLOG_LOG_MESSAGE_SIZE);

LogChannel::~LogChannel() {
    stop();
}

void LogChannel::push(LogRecord* rec) {    
    if (rec) {
        int level = rec->meta.severity;
        queue.push(rec);
        if (level <= FATL) {
            abort();
        }
    }
}

void LogChannel::start() {
    stop();
    logger_state = RUN;
    keepalive = true;
    workThread = std::thread([this]() { this->logging_loop(); });
}


void LogChannel::stop() {
    keepalive = false;
    if (workThread.joinable()) {
        workThread.join();
    }
    logger_state = SETUP;
}

void LogChannel::set_sink(std::unique_ptr<LogSink> sink_) {
    assert(logger_state == SETUP);
    if (sink_) {
        sink = std::move(sink_);
    } else {
        sink = std::unique_ptr<LogSink>(new NullSink);
    }
}

void LogChannel::set_threshold(ThresholdMap const& threshold_) {
    assert(logger_state == SETUP);
    thresholdMap = threshold_;
}

void LogChannel::logging_loop() {
    while (keepalive) {
        assert(logger_state == RUN);
        LogRecord* rec = queue.pop(WAIT);
        if (rec) {
            sink->record(rec);
            pool.deallocate(rec);
        }
    }
    // Shutdown. Drain the queue.
    LogRecord* head = queue.pop_all();
    while (head) {
        sink->record(head);
        LogRecord* cursor = head->next;
        pool.deallocate(head);
        head = cursor;
    }
}

}
