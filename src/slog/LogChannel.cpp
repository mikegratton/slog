#include "LogChannel.hpp"
#include "Signal.hpp"
#include "SlogError.hpp"
#include <cassert>
#include <chrono>
#include <cstdlib>

namespace slog
{

// This controls how fast the worker thread loop responds
// to signals to shut down.  50 ms is generally too short
// to notice at the console.
constexpr std::chrono::milliseconds WAIT{50};

LogChannel::LogChannel(std::shared_ptr<LogSink> sink_, ThresholdMap const& threshold_,
                       std::shared_ptr<LogRecordPool> pool_)
    : pool(pool_),
      threshold_map(threshold_),
      sink(sink_)
{
}

LogChannel::~LogChannel() { stop(); }


void LogChannel::push(LogRecord* node)
{
    if (node) {
        int level = node->meta().severity();
        queue.push(node);
        if (level <= FATL) {
            std::abort();
        }
    }
}

void LogChannel::start()
{
    if (get_signal_state() != SLOG_ACTIVE) {
        slog_error("Tried to start channel in STOPPED state\n");
    }
    work_thread = std::thread([this]() { this->logging_loop(); });
}

void LogChannel::stop()
{
    if (get_signal_state() == SLOG_ACTIVE) {
        set_signal_state(SLOG_STOPPED);
    }
    if (work_thread.joinable()) {
        work_thread.join();
    }
}

void LogChannel::logging_loop()
{
    assert(sink);
    assert(pool);
    while (logger_state() == RUN) {
        LogRecord* node = queue.pop(WAIT);
        if (node) {
            sink->record(*node);
            pool->free(node);
        }
    }

    // Shutdown. Drain the queue.
    LogRecord* head = queue.pop_all();
    while (head) {
        sink->record(*head);
        LogRecord* cursor = head->m_next;
        pool->free(head);
        head = cursor;
    }
    sink->finalize();
    notify_channel_done();
}

LogRecord* LogChannel::LogQueue::pop_all()
{
    std::unique_lock<std::mutex> guard(lock);
    LogRecord* popped = head;
    head = tail = nullptr;
    return popped;
}

void LogChannel::LogQueue::push(LogRecord* node)
{
    assert(node);
    node->m_next = nullptr;
    std::unique_lock<std::mutex> guard(lock);
    if (tail) {
        tail->m_next = node;
        tail = node;
    } else {
        tail = head = node;
    }
    guard.unlock();
    pending.notify_one();
}

LogRecord* LogChannel::LogQueue::pop(std::chrono::milliseconds wait)
{
    auto condition = [this]() -> bool { return head != nullptr; };
    LogRecord* popped = nullptr;

    std::unique_lock<std::mutex> guard(lock);
    if (pending.wait_for(guard, wait, condition)) {
        popped = head;
        head = head->m_next;
        if (nullptr == head) {
            tail = nullptr;
        }
        popped->m_next = nullptr;
    }
    return popped;
}

} // namespace slog
