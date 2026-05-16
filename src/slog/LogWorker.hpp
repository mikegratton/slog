#pragma once
#include "LogChannel.hpp"
#include "LogRecord.hpp"
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

namespace slog
{

/**
 * A worker thread and queue for writing records to sinks
 */
class LogWorker
{
  public:
    LogWorker();
    ~LogWorker();
    LogWorker(LogWorker const&) = delete;
    LogWorker(LogWorker&&) = delete;
    LogWorker& operator=(LogWorker) = delete;
    LogWorker& operator=(LogWorker&&) = delete;

    /**
     * Send a record to the to-be-logged queue. Thread-safe.
     */
    void push_to_queue(LogRecord* rec);

    /**
     * Add a log channel to the worker. This should only be done
     * in the stopped state.
     */
    void add_channel(int channel_id, std::shared_ptr<LogChannel> new_channel);

    /**
     * Get a channel. Thread safe.
     */
    std::shared_ptr<LogChannel> const& get_channel(int id);

    /**
     * Number of channels. Only thread safe in running state.
     * @note This is an O(N) operation
     */
    int channel_count() const;

    /**
     * Start the worker. If already started, this has no effect
     */
    void start();

    /**
     * Drain any queued messages.  Joint the work thread. Delete all channels.
     */
    void stop();

  private:
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

    /**
     * Worker function on work threads. This loops continuously processing the
     * LogQueue until the shutdown signal is sent.
     */
    void work();

    LogQueue record_queue;
    // We keep a vector of channels for O(1) lookup, even if many entries may be nullptr
    std::vector<std::shared_ptr<LogChannel>> channel_list;
    std::thread worker;
};

inline void LogWorker::push_to_queue(LogRecord* rec)
{
    if (rec) {
        int severity = rec->meta().severity();
        record_queue.push(rec);
        if (severity == FATL) {
            std::abort();
        }
    }
}

inline std::shared_ptr<LogChannel> const& LogWorker::get_channel(int id) { return channel_list[id]; }

} // namespace slog