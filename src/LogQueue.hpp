#pragma once
#include "LogRecordPool.hpp"
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace slog {

/**
 * A concurrent queue implemented as a linked list using the 
 * next pointer inside of LogRecord. This is mutex-synchronized,
 * allowing waiting on the condition variable so that the waiting
 * thread (the LogChannel worker) can be put to sleep and awoken
 * by the OS efficiently.
 */
class LogQueue
{
public:
    LogQueue() : mtail ( nullptr ), mhead ( nullptr ) { }

    void push ( RecordNode* record );

    RecordNode* pop(std::chrono::milliseconds wait);

    RecordNode* pop_all();

protected:
    std::mutex lock;
    std::condition_variable pending;

    RecordNode* mtail;
    RecordNode* mhead;
};
}
