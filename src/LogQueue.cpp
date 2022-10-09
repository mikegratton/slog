#include "LogQueue.hpp"

namespace slog {

LogRecord* LogQueue::pop_all() {
    std::unique_lock<std::mutex> guard(lock);
    LogRecord* popped = mtail;
    mtail = mhead = nullptr;
    return popped;
}


void LogQueue::push(LogRecord* record) {
    {
        std::unique_lock<std::mutex> guard(lock);
        if (mhead) {
            mhead->next = record;
            mhead = record;
        } else {
            mhead = mtail = record;
        }        
    }
    pending.notify_one();
}

LogRecord* LogQueue::pop(std::chrono::milliseconds wait) {
    auto condition = [this]() -> bool {return mtail != nullptr; };
    LogRecord* popped ;
    {
        std::unique_lock<std::mutex> guard(lock);
        if (pending.wait_for(guard, wait, condition)) {
            popped = mtail;
            mtail = popped->next;
            if (nullptr == mtail) {
                mhead = nullptr;
            }
            popped->next = nullptr;
            return popped;
        } else {
            return nullptr;
        }
    }
}
}
