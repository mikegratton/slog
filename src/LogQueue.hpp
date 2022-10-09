#pragma once
#include "LogRecord.hpp"
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace slog {

class LogQueue
{
public:
    LogQueue() : mhead ( nullptr ), mtail ( nullptr ) { }

    void push ( LogRecord* record );

    LogRecord* pop(std::chrono::milliseconds wait);

    LogRecord* pop_all();

protected:
    std::mutex lock;
    std::condition_variable pending;

    LogRecord* mhead;
    LogRecord* mtail;
};
}
