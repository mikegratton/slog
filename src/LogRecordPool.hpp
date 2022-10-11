#pragma once
#include "LogRecord.hpp"
#ifndef SLOG_LOCK_FREE
#include <mutex>
namespace slog {
/*
 * This is a stack of records -- a pool allocator for a single type
 * of object, a LogRecord.
 */

class MutexLogRecordPool {
public:
    MutexLogRecordPool(long max_size, long message_size);

    ~MutexLogRecordPool();

    /**
     * Pop a record from the stack. If the stack is currently empty, \
     * this returns nullptr
     */
    LogRecord* allocate();

    /**
     * Push a record back onto the stack.
     */
    void deallocate(LogRecord* record);

protected:
    std::mutex mlock;

    long mchunkSize;
    long mmessageSize;
    long mchunks;

    LogRecord* mcursor; // head of the stack
    LogRecord* mpool; // Start of heap allocated region
};
using LogRecordPool = MutexLogRecordPool;
}

#else

// Lock-free experimental pool
#include <atomic>
namespace slog {

class LfLogRecordPool {
public:
    LfLogRecordPool(long max_size, long message_size);

    ~LfLogRecordPool();

    LogRecord* allocate();

    void deallocate(LogRecord* record);

protected:

    using AtomicPtr = std::atomic<LogRecord*> ;

    long mchunkSize;
    long mmessageSize;
    long mchunks;

    AtomicPtr mcursor;  // head of the stack
    LogRecord* mpool; // Start of heap allocated region
};
using LogRecordPool = LfLogRecordPool;
}
#endif
