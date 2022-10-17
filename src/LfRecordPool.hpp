#pragma once
#include "LogConfig.hpp"
#if SLOG_LOCK_FREE_POOL
#include "LogRecord.hpp"
#include <atomic>
#include <cstdint>

namespace slog {

struct RecordNode;

class RecordPtr {
protected:    
    uint32_t moffset; // Offset of ptr from pool start
    uint32_t mtag; // modification tag for ABA'
    
public:
    RecordPtr() : moffset(0), mtag(0) { }
    
    RecordPtr(RecordNode* node, char* origin, uint32_t tag_)         
    {
        set(node, origin, tag_);
    }
        
    operator RecordNode*() { return reinterpret_cast<RecordNode*>(this); }
    RecordPtr& operator=(RecordNode* ptr);
    RecordNode* get(char* origin);
    RecordPtr& set(RecordNode* node, char* origin, int tag);
    
    bool operator==(RecordNode const* ptr) const;
    operator bool() const { return moffset != 0; }
    uint32_t tag() const { return mtag; }
};

struct RecordNode {
    RecordPtr next;
    LogRecord rec; // Payload
};


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
