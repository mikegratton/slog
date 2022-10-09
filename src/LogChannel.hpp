#pragma once

#include "LogRecord.hpp"
#include "LogQueue.hpp"
#include "LogRecordPool.hpp"
#include "ThresholdMap.hpp"
#include "LogSink.hpp"

#include <thread>
#include <atomic>

namespace slog {

// All channels draw record from a central pool
extern LogRecordPool s_allocator;

class LogChannel {
public:
    LogChannel(LogRecordPool& pool_ = s_allocator) : runMode(false), pool(pool_) { }
    ~LogChannel();

    LogChannel& operator=(LogChannel const&) = delete;
    LogChannel(LogChannel const&) = delete;

    /**
     * Start the worker thread. Exits setup mode.
     */
    void start();

    /**
     * Drain the queue into the sink. Stop the worker thread.
     */
    void stop();

    /**
     * Attempt to allocate a new record from the pool.
     * Will return nullptr if the pool is exhausted.
     */
    LogRecord* allocate() { return pool.allocate(); }
    
    /**
     * Send a record to the worker thread. Ownership of
     * this pointer transfered to the queue.
     */
    void push(LogRecord* rec);
    
    /**
     * Check the severity threshold for the given tag
     */
    int threshold(char const* tag) { return thresholdMap[tag]; }
    
    //////////////////////////////////////////////////////////////////
    // SETUP MODE ONLY
    void set_sink(std::unique_ptr<LogSink> sink_);
    
    void set_threshold(ThresholdMap const& threshold_);
    
protected:
    bool runMode; 
    
    LogRecordPool& pool;
    LogQueue queue;
    ThresholdMap thresholdMap;

    std::unique_ptr<LogSink> sink;
    std::thread workThread;
    std::atomic_bool keepalive;
};

}
