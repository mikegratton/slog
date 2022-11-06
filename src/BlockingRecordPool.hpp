#pragma once

#include "LogRecord.hpp"
#include <mutex>
#include <condition_variable>

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

class BlockingRecordPool {
public:
    BlockingRecordPool(long max_size, long message_size);

    ~BlockingRecordPool();

    /**
     * Pop a record from the stack. If the stack is currently empty, \
     * this blocks until a record is available
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
    std::condition_variable mnonempty;
};
}
