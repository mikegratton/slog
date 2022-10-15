#include "LogChannel.hpp"
#include <cassert>
#include <chrono>

namespace slog {

constexpr std::chrono::milliseconds WAIT{50};

LogRecordPool s_allocator(SLOG_LOG_POOL_SIZE, SLOG_LOG_MESSAGE_SIZE);

LogChannel::~LogChannel() {
    stop();
}


void LogChannel::push(RecordNode* node) {    
    if (node) {
        int level = node->rec.meta.severity;
        queue.push(node);
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
        RecordNode* node = queue.pop(WAIT);
        if (node) {
            sink->record(node->rec);
            pool.put(node);
        }
    }
    // Shutdown. Drain the queue.
    RecordNode* head = queue.pop_all();
    while (head) {
        sink->record(head->rec);
        RecordNode* cursor = head->next;
        pool.put(head);
        head = cursor;
    }
}


RecordNode* LogChannel::LogQueue::pop_all() {
    std::unique_lock<std::mutex> guard(lock);
    RecordNode* popped = mhead;
    mhead = mtail = nullptr;
    return popped;
}


void LogChannel::LogQueue::push(RecordNode* node) {
    if (nullptr == node) {
        return;
    }
    node->next = nullptr;
    std::unique_lock<std::mutex> guard(lock);
    if (mtail) {        
        mtail->next = node;
        mtail = node;
    } else {
        mtail = mhead = node;
    }
    guard.unlock();
    pending.notify_one();
}

RecordNode* LogChannel::LogQueue::pop(std::chrono::milliseconds wait) {
    auto condition = [this]() -> bool {return mhead != nullptr; };
    RecordNode* popped = nullptr;

    std::unique_lock<std::mutex> guard(lock);
    if (pending.wait_for(guard, wait, condition)) {
        popped = mhead;        
        mhead = mhead->next;        
        if (nullptr == mhead) {
            mtail = nullptr;
        }
    }

    return popped;
}

}
