#pragma once
#include "LogConfig.hpp"
#if SLOG_LOCK_FREE_POOL
#include "LfRecordPool.hpp"
#else

#include "LogRecord.hpp"
#include <mutex>
namespace slog {
    
struct RecordNode;

using RecordPtr = RecordNode*;

struct RecordNode {    
    RecordPtr next;
    LogRecord rec; // Payload
};

/*
 * This is a stack of records -- a pool for a single type
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
    RecordNode* take();

    /**
     * Push a record back onto the stack.
     */    
    void put(RecordNode* record);

    // Count items on the pool. Not thread-safe
    long count() const;
    
protected:
    mutable std::mutex mlock;

    long mchunkSize;    
    long mchunks;

    RecordPtr mcursor; // head of the stack
    char* mpool; // Start of heap allocated region
};
using LogRecordPool = MutexLogRecordPool;
}
#endif
