#pragma once
#include "LogRecord.hpp"
#include <mutex>
#include <condition_variable>

namespace slog {
    
struct RecordNode;

using RecordPtr = RecordNode*;

struct RecordNode {    
    RecordPtr next;
    RecordPtr jumbo;
    LogRecord rec; // Payload
};

class PoolMemory;

enum LogRecordPoolPolicy {
    ALLOCATE,
    BLOCK,
    DISCARD
};

/*
 * This is a stack of records -- a pool for a single type
 * of object, a LogRecord.
 */
class LogRecordPool {
public:
    LogRecordPool(LogRecordPoolPolicy i_policy, long i_pool_alloc_size, long i_message_size,
        long i_max_blocking_time_ms = 50);

    ~LogRecordPool();

    /**
     * Pop a record from the stack. If the stack is currently empty, \
     * this allocates more memory
     */
    RecordNode* take();

    /**
     * Push a record back onto the stack.
     */    
    void put(RecordNode* record);

    // Count items on the pool. Not thread-safe
    long count() const;
    
protected:
    
    void allocate();
    
    mutable std::mutex m_lock;
    std::condition_variable m_nonempty;

    LogRecordPoolPolicy m_policy;
    long m_max_blocking_time_ms;
    long m_chunkSize;    
    long m_chunks;

    RecordPtr m_cursor; // head of the stack
    PoolMemory* m_pool; // Start of heap allocated region
};
}
