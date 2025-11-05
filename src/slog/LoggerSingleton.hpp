#pragma once
#include "LogChannel.hpp"
#include "LogSetup.hpp"
#include "slog/LogRecordPool.hpp"
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
    static std::size_t channel_count();

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
     * Internal log start function.
     */
    static void start_all_channels();

    /**
     * Internal log stop function.
     */
    static void stop_all_channels();

    /**
     * @brief When slog is "stopped", we may still install a channel to log to
     * the console. This method handles that logic.
     */
    static void setup_stopped_channel();

  private:
    Logger();

    ~Logger();

    static Logger& instance();

    void do_setup_stopped_channel();

    static std::shared_ptr<LogRecordPool> make_default_pool();

    std::vector<LogChannel> backend;
};


/// (For debugging the logger) Check that all pool records are either free or in
/// a queue. Not thread safe.
long get_pool_missing_count();

///////////////////////////////////////////////////////////////////////////////////////////////////////

inline std::size_t Logger::channel_count() { return instance().backend.size(); }

inline LogChannel& Logger::get_channel(int channel)
{
    if (channel < 0 || channel >= instance().backend.size()) {
        channel = DEFAULT_CHANNEL;
    }
    return instance().backend[channel];
}


} // namespace detail
} // namespace slog
