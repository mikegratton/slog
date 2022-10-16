#pragma once
#include "LogChannel.hpp"
#include "LogRecord.hpp"
#include "LogRecordPool.hpp"
#include "ThresholdMap.hpp"
#include "LogSink.hpp"

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace slog {

/**
 * Principle worker of the slog system. The LogChannel provides
 * a thread-safe interface to the log sink and the pool.
 *
 * This creates a thread on start() that watches a queue.  When
 * messages arrive, the thread dequeues them, sending them to
 * the LogSink. They're then deallocated back to the pool.
 * stop() drains the queue to the sink, then joins the worker
 * thread.
 *
 * Conceptually, LogChannel has two states, SETUP and RUN.
 * In SETUP, calls to set_sink() and set_threshold() are legal.
 * In RUN, calls to push() are legal.
 *
 */
class LogChannel {
public:
    LogChannel();
    ~LogChannel();

    LogChannel& operator=(LogChannel const&) = delete;
    LogChannel(LogChannel const&) = delete;

    /**
     * Start the worker thread. Transition to RUN state
     */
    void start();

    /**
     * Drain the queue into the sink. Stop the worker thread.
     */
    void stop();


    /**
     * Check the severity threshold for the given tag. Thread safe
     * in RUN mode.
     */
    int threshold(char const* tag) { return thresholdMap[tag]; }

    /**
     * Attempt to allocate a new record from the pool.
     * Will return nullptr if the pool is exhausted.
     * Thread safe.
     */
    RecordNode* allocate() { return pool.take(); }


    ///////////////////////////////////////////////////////////////////
    // RUN STATE ONLY
    /**
     * Send a record to the worker thread. Ownership of
     * this pointer transfered to the queue. Thread safe.
     */
    void push(RecordNode* rec);

    //////////////////////////////////////////////////////////////////
    // SETUP STATE ONLY
    void set_sink(std::unique_ptr<LogSink> sink_);

    void set_threshold(ThresholdMap const& threshold_);
    
    long allocator_count() const { return pool.count(); }

protected:
    // Internal work function call on the workThread
    void logging_loop();


    /**
     * A concurrent queue implemented as a linked list using the
     * next pointer inside of LogRecord. This is mutex-synchronized,
     * allowing waiting on the condition variable so that the waiting
     * thread (the LogChannel worker) can be put to sleep and awoken
     * by the OS efficiently.
     */
    class LogQueue {
    public:
        LogQueue() : mtail(nullptr), mhead(nullptr) { }

        void push(RecordNode* record);

        RecordNode* pop(std::chrono::milliseconds wait);

        RecordNode* pop_all();

    protected:
        std::mutex lock;
        std::condition_variable pending;

        RecordNode* mtail;
        RecordNode* mhead;
    };

    enum State {
        SETUP,
        RUN
    };
    State logger_state() { return (keepalive? RUN : SETUP); }

// These object have only thread-safe calls
    LogRecordPool pool;
    LogQueue queue;

// This state should not be mutated in RUN mode
    ThresholdMap thresholdMap;
    std::unique_ptr<LogSink> sink;

    std::thread workThread;
    std::atomic_bool keepalive;
};

}
