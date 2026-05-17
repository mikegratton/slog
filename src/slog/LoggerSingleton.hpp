#pragma once
#include "LogChannel.hpp"
#include "LogRecordPool.hpp"
#include "LogSetup.hpp"
#include "LogWorker.hpp"
#include "slog/LogRecord.hpp"
#include <memory>
#include <vector>

namespace slog
{
namespace detail
{
/**
 * @brief The Logger singleton manages logger channels, record pools, and
 * signals.
 *
 * All public methods are thread safe
 */
class Logger
{
  public:
    /**
     * @brief Number of active channels
     */
    static int channel_count();

    /**
     * @brief Number of active workers
     */
    static int worker_count();

    /**
     *  @brief Set up a channel for each LogConfig.
     *
     * This will stop the logger when called.
     */
    static void setup_channels(std::vector<LogConfig>& config);

    /**
     * @brief Get a ref to the requested channel.
     *
     * If channel is invalid, get the DEFAULT_CHANNEL
     */
    static LogChannel& get_channel(int channel);

    /**
     * Push a record to the backend
     */
    static void push_to_sink(LogRecord* record);

    /**
     * Internal log start function.
     */
    static void start();

    /**
     * Internal log stop function.
     */
    static void stop();

  private:
    Logger();

    static Logger& instance();

    void setup_default_channel();

    static std::shared_ptr<LogRecordPool> make_default_pool();

    /// nominal number of workers
    int num_worker;

    /// Lists the worker for each channel id
    std::vector<std::shared_ptr<LogWorker>> channel_worker;
};

/// (For debugging the logger) Check that all pool records are either free or in
/// a queue. Not thread safe.
long get_pool_missing_count();

///////////////////////////////////////////////////////////////////////////////////////////////////////

inline LogChannel& Logger::get_channel(int channel)
{
    if (channel_count() == 0) {
        instance().setup_default_channel();
    }
    if (channel < 0 || channel >= channel_count()) {
        channel = DEFAULT_CHANNEL;
    }
    return *instance().channel_worker[channel]->get_channel(channel);
}

inline void Logger::push_to_sink(LogRecord* record)
{
    instance().channel_worker[record->meta().channel()]->push_to_queue(record);
}

} // namespace detail
} // namespace slog
