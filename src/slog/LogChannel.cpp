#include "LogChannel.hpp"
#include <cstdlib>
#include <cassert>
#include <chrono>

namespace slog {

// This controls how fast the worker thread loop responds
// to signals to shut down.  50 ms is generally too short
// to notice at the console.
constexpr std::chrono::milliseconds WAIT{50};

LogChannel::LogChannel(std::shared_ptr<LogSink> sink_, ThresholdMap const& threshold_,
                       std::shared_ptr<LogRecordPool> pool_)
    : pool(pool_), thresholdMap(threshold_), sink(sink_), keepalive(false)
{
}

LogChannel::~LogChannel()
{
    stop();
}

LogChannel::LogChannel(LogChannel const& i_rhs) : LogChannel(i_rhs.sink, i_rhs.thresholdMap, i_rhs.pool)
{
}

void LogChannel::push(RecordNode* node)
{
    if (node) {
        int level = node->rec.meta.severity;
        queue.push(node);
        if (level <= FATL) { abort(); }
    }
}

void LogChannel::start()
{
    stop();
    keepalive = true;
    workThread = std::thread([this]() { this->logging_loop(); });
}

void LogChannel::stop()
{
    keepalive = false;
    if (workThread.joinable()) { workThread.join(); }
}

void LogChannel::logging_loop()
{
    assert(sink);
    assert(pool);
    while (keepalive) {
        RecordNode* node = queue.pop(WAIT);
        if (node) {
            sink->record(node->rec);
            pool->free(node);
        }
    }
    // Shutdown. Drain the queue.
    RecordNode* head = queue.pop_all();
    while (head) {
        sink->record(head->rec);
        RecordNode* cursor = head->next;
        pool->free(head);
        head = cursor;
    }
}

RecordNode* LogChannel::LogQueue::pop_all()
{
    std::unique_lock<std::mutex> guard(lock);
    RecordNode* popped = mhead;
    mhead = mtail = nullptr;
    return popped;
}

void LogChannel::LogQueue::push(RecordNode* node)
{
    assert(node);
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

RecordNode* LogChannel::LogQueue::pop(std::chrono::milliseconds wait)
{
    auto condition = [this]() -> bool { return mhead != nullptr; };
    RecordNode* popped = nullptr;

    std::unique_lock<std::mutex> guard(lock);
    if (pending.wait_for(guard, wait, condition)) {
        popped = mhead;
        mhead = mhead->next;
        if (nullptr == mhead) { mtail = nullptr; }
        popped->next = nullptr;
    }
    return popped;
}

}  // namespace slog
