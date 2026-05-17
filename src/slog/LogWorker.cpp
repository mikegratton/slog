#include "LogWorker.hpp"
#include "LogChannel.hpp"
#include "LogRecord.hpp"
#include "Signal.hpp"
#include <cassert>

namespace slog
{
// This controls how fast the worker thread loop responds
// to signals to shut down.  50 ms is generally too short
// to notice at the console.
constexpr std::chrono::milliseconds WAIT{50};

LogWorker::LogWorker() = default;

LogWorker::~LogWorker() { stop(); }

void LogWorker::stop()
{
    if (!worker.joinable()) {
        channel_list.clear();
        return;
    }
    if (get_signal_state() == SLOG_ACTIVE) {
        set_signal_state(SLOG_STOPPED);
    }    
    worker.join();    
    channel_list.clear();
}

void LogWorker::add_channel(int channel_id, std::shared_ptr<LogChannel> channel)
{
    assert(!worker.joinable());
    if (channel_id >= (int) channel_list.size()) {
        channel_list.resize(channel_id+1);
    }
    channel_list[channel_id] = std::move(channel);
}

void LogWorker::start()
{
    if (worker.joinable()) {
        return;
    }
    notify_worker_starting();
    worker = std::thread([this]() { this->work(); });
}

void LogWorker::work()
{
    // Note: If we encounter channel_id's we don't know, there's little we can do.
    // If we ignore the node, then we leak the resource because we don't know which
    // pool to return it to.
    while (get_signal_state() == SLOG_ACTIVE) {
        LogRecord* node = record_queue.pop(WAIT);
        if (node) {
            int channel_id = node->meta().channel();
            assert(channel_id >= 0 && channel_id < (int)channel_list.size());
            auto& channel = channel_list[channel_id];
            assert(channel);
            channel->send_to_sink(node);
        }
    }
    // Drain the queue
    LogRecord* head = record_queue.pop_all();
    while (head) {
        int channel_id = head->meta().channel();
        assert(channel_id >= 0 && channel_id < (int)channel_list.size());
        auto& channel = channel_list[channel_id];
        assert(channel);
        LogRecord* next = head->m_next;
        channel->send_to_sink(head);
        head = next;
    }
    for (auto& channel : channel_list) {
        if (channel) {
            channel->finalize();
        }
    }
    notify_worker_stopping();
}

LogRecord* LogWorker::LogQueue::pop_all()
{
    std::unique_lock<std::mutex> guard(lock);
    LogRecord* popped = head;
    head = tail = nullptr;
    return popped;
}

void LogWorker::LogQueue::push(LogRecord* node)
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

LogRecord* LogWorker::LogQueue::pop(std::chrono::milliseconds wait)
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
        guard.unlock();
        popped->m_next = nullptr;
    }
    return popped;
}

int LogWorker::channel_count() const
{
    int num_channel = 0;
    for (auto const& channel : channel_list) {
        if (channel) {
            num_channel++;
        }
    }
    return num_channel;
}

} // namespace slog