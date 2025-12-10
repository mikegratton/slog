#pragma once
#include <condition_variable>
#include <mutex>
#include "SlogConfig.hpp"
#include "LogRecord.hpp"

namespace slog
{

using NodePtr = LogRecord*;

class PoolMemory;

enum LogRecordPoolPolicy { ALLOCATE, BLOCK, DISCARD };

/**
 * @brief A memory pool for unused log records
 *
 * This is a stack of records -- a pool for a single type of object, a
 * LogRecord.
 *
 * @param policy -- what to do when the pool is exhausted
 * @param pool_alloc_size -- maximum memory to allocate per allocation (if the
 * policy is BLOCK or DISCARD, only one allocation is performed)
 * @param message_size -- Size (in bytes) of one message node. (Longer
 * messages will require multiple nodes)
 * @param max_blocking_time_ms -- Max milliseconds to block in BLOCK policy
 * mode while waiting for blank records to be returned to the pool. This has no
 * effect in ALLOCATE or DISCARD mode.
 */
class LogRecordPool
{
  public:
    LogRecordPool(LogRecordPoolPolicy policy,
                  long pool_alloc_size = DEFAULT_POOL_RECORD_COUNT *
                                         (DEFAULT_RECORD_SIZE + sizeof(LogRecord)),
                  long message_size = DEFAULT_RECORD_SIZE, long max_blocking_time_ms = 50);

    ~LogRecordPool();
    LogRecordPool(LogRecordPool const&) = delete;
    LogRecordPool(LogRecordPool&&) = delete;
    LogRecordPool& operator=(LogRecordPool const&) = delete;
    LogRecordPool& operator=(LogRecordPool&&) = delete;

    /**
     * Pop a record from the stack. If the stack is currently empty,
     * the LogRecordPoolPolicy determines what happens.
     */
    LogRecord* allocate();

    /**
     * Return a record to the pool as free.
     */
    void free(LogRecord* record);

    // Count items in the pool. Not thread-safe
    long count() const;

  private:
    void acquire_blank_records();

    mutable std::mutex lock;
    std::condition_variable nonempty;

    LogRecordPoolPolicy policy;
    long max_blocking_time_ms;
    long message_size;
    long chunks;

    NodePtr head;   // head of the stack
    PoolMemory* pool; // Start of heap allocated region
};
} // namespace slog
