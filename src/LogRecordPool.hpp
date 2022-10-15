#pragma once
#include "LogRecord.hpp"

namespace slog {

#ifndef SLOG_LOCK_FREE
    
using RecordPtr = RecordNode*;

#else

struct RecordPtr {
    RecordNode* ptr;
    long atag;
    
    RecordPtr() = default;
    RecordPtr(RecordNode* node) : ptr(node), atag(0) { }
    RecordPtr& operator=(RecordNode* node) { ptr = node; atag++; return *this; }
    RecordNode* operator->() { return ptr; }
    RecordNode const* operator->() const { return ptr; }
    operator bool() const { return ptr != nullptr; }
    operator RecordNode*() { return ptr; }
    void tag() { atag++; }
};

#endif


struct RecordNode {    
    RecordPtr next;
    int channel; // Marks which channel a record belongs to
    LogRecord rec; // Payload
};

}

#ifndef SLOG_LOCK_FREE

#include <mutex>
namespace slog {
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

#else

// Lock-free experimental pool
#include <atomic>
namespace slog {

class LfLogRecordPool {
public:
    LfLogRecordPool(long max_size, long message_size);

    ~LfLogRecordPool();

    RecordNode* take();

    void put(RecordNode* record);

    // Count items on the pool. Not thread-safe
    long count() const;
    
protected:

    using AtomicPtr = std::atomic<RecordPtr> ;

    long mchunkSize;
    long mmessageSize;
    long mchunks;

    AtomicPtr mcursor;  // head of the stack
    char* mpool; // Start of heap allocated region
};
using LogRecordPool = LfLogRecordPool;
}
#endif
