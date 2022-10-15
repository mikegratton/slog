#include "LogQueue.hpp"

namespace slog {

RecordNode* LogQueue::pop_all() {
    std::unique_lock<std::mutex> guard(lock);
    RecordNode* popped = mhead;
    mhead = mtail = nullptr;
    return popped;
}


void LogQueue::push(RecordNode* node) {
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

RecordNode* LogQueue::pop(std::chrono::milliseconds wait) {
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
