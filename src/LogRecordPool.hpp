#pragma once
#include "LogRecord.hpp"
#include <mutex>

namespace slog {

class LogRecordPool 
{
public:
    LogRecordPool ( long max_size = SLOG_LOG_POOL_SIZE, long message_size = SLOG_LOG_MESSAGE_SIZE);

    ~LogRecordPool();

    LogRecord* allocate();

    void deallocate ( LogRecord* record );

protected:
    std::mutex mlock;

    long mchunkSize;
    long mmessageSize;
    long mchunks;

    LogRecord* mcursor;
    LogRecord* mpool;
};
}
