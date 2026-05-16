#pragma once

#include "LogRecord.hpp"
#include "LogRecordPool.hpp"
#include "LogSink.hpp"
#include "ThresholdMap.hpp"

namespace slog
{

/**
 * @brief Principle worker of the slog system.
 *
 * The LogChannel provides a thread-safe interface to the log sink and the pool.
 *
 * This creates a thread on start() that watches a queue.  When
 * messages arrive, the thread dequeues them, sending them to
 * the LogSink. They're then deallocated back to the pool.
 * stop() drains the queue to the sink, then joins the worker
 * thread.
 *
 * Conceptually, LogChannel has two states, SETUP and RUN.
 * In SETUP, calls to set_sink(), set_threshold(), and set_pool are legal.
 * In RUN, calls to push() are legal.
 *
 */
class LogChannel
{
  public:
    /**
     * Ctor.
     */
    LogChannel(std::shared_ptr<LogSink> sink, ThresholdMap const& threshold, std::shared_ptr<LogRecordPool> pool);

    /**
     * Dtor. Calls stop() so that all enqued messages will
     * be sent to the sink before returning.
     */
    ~LogChannel();

    LogChannel(LogChannel const&) = delete;
    LogChannel& operator=(LogChannel const&) = delete;
    LogChannel(LogChannel&&) noexcept = delete;
    LogChannel& operator=(LogChannel&&) noexcept = delete;

    /**
     * Check the severity threshold for the given tag. Thread safe in RUN mode.
     */
    int threshold(char const* tag) const { return threshold_map[tag]; }

    /**
     * Attempt to grab a new record from the pool. Will return nullptr if the
     * pool is exhausted. Thread safe.
     */
    LogRecord* get_fresh_record() { return pool->allocate(); }

    /**
     * Return a record to the pool. Thread safe.
     */
    void dispose_of_record(LogRecord* record) { pool->free(record); }

    /**
     * Send the record to the sink. Then, this function frees the
     * record. DO NOT USE rec after calling this function.
     */
    void send_to_sink(LogRecord* rec);

    /**
     * @brief Obtain the number of free records in the pool
     */
    long pool_free_count() const { return pool->count(); }

    /**
     * Send the finalize signal to the sink
     */
    void finalize();

  private:
    // These object have only thread-safe calls
    std::shared_ptr<LogRecordPool> pool;

    // This state should not be mutated in RUN mode
    ThresholdMap threshold_map;
    std::shared_ptr<LogSink> sink;
};

} // namespace slog
