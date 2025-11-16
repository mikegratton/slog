#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>

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
     * Start the worker thread. Transition to RUN state. Thread safe.
     */
    void start();

    /**
     * Drain the queue into the sink. Stop the worker thread.
     * @note This will stop *all* channels. Thread safe.
     */
    void stop();

    /**
     * Check the severity threshold for the given tag. Thread safe in RUN mode.
     */
    int threshold(char const* tag) { return threshold_map[tag]; }

    /**
     * Attempt to grab a new record from the pool. Will return nullptr if the
     * pool is exhausted. Thread safe.
     */
    LogRecord* get_fresh_record() { return pool->allocate(); }

    /**
     * Send a record to the worker thread. Ownership of this pointer transfered
     * to the queue. Thread safe. RUN STATE ONLY
     */
    void push(LogRecord* rec);

    /*
     * For debugging. Compare the number of messages in the pool to the expected
     * number. Not thread safe. SETUP STATE ONLY
     */
    long allocator_count() const { return pool->count(); }

  protected:
    // Internal work function call on the workThread
    void logging_loop();

    /**
     * A concurrent queue implemented as a linked list using the next pointer
     * inside of LogRecord. This is mutex-synchronized, allowing waiting on the
     * condition variable so that the waiting thread (the LogChannel worker) can
     * be put to sleep and awoken by the OS efficiently.
     */
    class LogQueue
    {
      public:
        LogQueue()
            : tail(nullptr),
              head(nullptr)
        {
        }

        void push(LogRecord* record);

        LogRecord* pop(std::chrono::milliseconds wait);

        LogRecord* pop_all();

      private:
        std::mutex lock;
        std::condition_variable pending;

        LogRecord* tail;
        LogRecord* head;
    };

  private:
    enum State { SETUP, RUN };
    State logger_state();

    // These object have only thread-safe calls
    std::shared_ptr<LogRecordPool> pool;
    LogQueue queue;

    // This state should not be mutated in RUN mode
    ThresholdMap threshold_map;
    std::shared_ptr<LogSink> sink;

    // Worker thread stuff
    std::thread work_thread;
};

} // namespace slog
