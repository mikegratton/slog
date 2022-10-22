#pragma once
#include "LogConfig.hpp"
#if SLOG_LOCK_FREE_POOL
#include "LogRecord.hpp"
#include <atomic>
#include <cstdint>

namespace slog {

struct RecordNode;
using RecordPtr = RecordNode*;

struct RecordNode {
    RecordPtr next;
    LogRecord rec; 
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
    
    using AtomicPtr = std::atomic<RecordPtr>;
    
    void link(RecordPtr newTop, RecordPtr end);
    RecordPtr to_full_ptr(RecordPtr packed) const;
    RecordPtr to_packed(RecordPtr full, uint32_t tag = 0) const;
    RecordPtr& packed_next(RecordPtr& packed) const;
    void increment_packed(RecordPtr& packed) const;
    bool is_packed_nullptr(RecordPtr const& packed) const;

    uint32_t mchunkSize;
    uint32_t mmessageSize;
    uint32_t mchunks;

    AtomicPtr mcursor;  // head of the stack
    char* mpool; // Start of heap allocated region
};
using LogRecordPool = LfLogRecordPool;
}
#endif
