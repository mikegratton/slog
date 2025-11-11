#pragma once
#include <condition_variable>
#include <mutex>

#include "LogRecord.hpp"

namespace slog {

struct RecordNode;

using NodePtr = RecordNode*;

// The layout of this struct is important. Do
// not change it without fixing the functions
// below.
struct RecordNode {
    NodePtr next;
    LogRecord rec;  // Payload
};

/// Convert a LogRecord ptr into a RecordPtr by doing some arithmetic
NodePtr toNodePtr(LogRecord const* i_rec);

/// Link two records together
void attach(NodePtr io_rec, NodePtr i_jumbo);

class PoolMemory;

enum LogRecordPoolPolicy { ALLOCATE, BLOCK, DISCARD };

/**
 * @brief A memory pool for unused log records
 *
 * This is a stack of records -- a pool for a single type of object, a
 * LogRecord.
 *
 * @param i_policy -- what to do when the pool is exhausted
 * @param i_pool_alloc_size -- maximum memory to allocate per allocation (if the
 * policy is BLOCK or DISCARD, only one allocation is performed)
 * @param i_message_size -- Size (in bytes) of one message node. (Longer
 * messages will require multiple nodes)
 * @param i_max_blocking_time_ms -- Max milliseconds to block in BLOCK policy
 * mode while waiting for blank records to be returned to the pool. This has no
 * effect in ALLOCATE or DISCARD mode.
 */
class LogRecordPool {
   public:
    LogRecordPool(LogRecordPoolPolicy i_policy, long i_pool_alloc_size, long i_message_size,
                  long i_max_blocking_time_ms = 50);

    ~LogRecordPool();

    /**
     * Pop a record from the stack. If the stack is currently empty,
     * the LogRecordPoolPolicy determines what happens.
     */
    RecordNode* allocate();

    /**
     * Return a record to the pool as free.
     */
    void free(RecordNode* record);

    // Count items in the pool. Not thread-safe
    long count() const;

   protected:
    void acquire_blank_records();

    mutable std::mutex m_lock;
    std::condition_variable m_nonempty;

    LogRecordPoolPolicy m_policy;
    long m_max_blocking_time_ms;
    long m_chunkSize;
    long m_chunks;

    NodePtr m_cursor;    // head of the stack
    PoolMemory* m_pool;  // Start of heap allocated region
};
}  // namespace slog
