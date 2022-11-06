#pragma once
#include "LogRecord.hpp"
#include <mutex>


namespace slog {
    
struct RecordNode;

using RecordPtr = RecordNode*;

struct RecordNode {    
    RecordPtr next;
    LogRecord rec; // Payload
};

class PoolMemory;

/*
 * This is a stack of records -- a pool for a single type
 * of object, a LogRecord.
 */
class AllocatingRecordPool {
public:
    AllocatingRecordPool(long max_size, long message_size);

    ~AllocatingRecordPool();

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
    
    mutable std::mutex mlock;

    long mchunkSize;    
    long mchunks;

    RecordPtr mcursor; // head of the stack
    PoolMemory* mpool; // Start of heap allocated region
};
}
