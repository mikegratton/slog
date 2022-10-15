#pragma once

#include "LogRecord.hpp"
#include "LogQueue.hpp"
#include "LogRecordPool.hpp"
#include "ThresholdMap.hpp"
#include "LogSink.hpp"

#include <thread>
#include <atomic>

namespace slog {

// All channels draw record from a central pool by default
extern LogRecordPool s_allocator;

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
    LogChannel(LogRecordPool& pool_ = s_allocator) : logger_state(SETUP), pool(pool_) { }
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
    
protected:
    // Internal work function call on the workThread
    void logging_loop();
    
    enum State {
        SETUP,
        RUN
    } logger_state;
    
    // These object have only thread-safe calls
    LogRecordPool& pool;
    LogQueue queue;
    
    // This state should not be mutated in RUN mode
    ThresholdMap thresholdMap;
    std::unique_ptr<LogSink> sink;
    
    std::thread workThread;
    std::atomic_bool keepalive;
};

}
