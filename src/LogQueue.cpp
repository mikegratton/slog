#include "LogQueue.hpp"

namespace slog {

LogRecord* LogQueue::pop_all() {
    std::unique_lock<std::mutex> guard(lock);
    LogRecord* popped = mhead;
    mhead = mtail = nullptr;
    return popped;
}


void LogQueue::push(LogRecord* record) {

    std::unique_lock<std::mutex> guard(lock);
    if (mtail) {
        mtail->next = record;
        mtail = record;
    } else {
        mtail = mhead = record;
    }
    guard.unlock();
    pending.notify_one();
}

LogRecord* LogQueue::pop(std::chrono::milliseconds wait) {
    auto condition = [this]() -> bool {return mhead != nullptr; };
    LogRecord* popped = nullptr;

    std::unique_lock<std::mutex> guard(lock);
    if (pending.wait_for(guard, wait, condition)) {
        popped = mhead;        
        mhead = mhead->next;
        popped->next = nullptr;
        if (nullptr == mhead) {
            mtail = nullptr;
        }
    }

    return popped;
}
}
