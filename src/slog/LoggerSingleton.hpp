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
 */
class Logger
{

  public:
    /**
     * @brief Number of active channels
     */
    static std::size_t channel_count();

    /**
     *  @brief Set up a channel for each log config
     */
    static void setup_channels(std::vector<LogConfig> const& config);

    /**
     * @brief Get a ref to the requested channel.
     *
     * If channel is invalid, get the DEFAULT_CHANNEL
     */
    static LogChannel& get_channel(int channel);


    /**
     * Internal log start function. Also installs thstop_all_channelse
     * signal handler.
     */
    static void start_all_channels();

    /**
     * Inverse of start_all_channels
     */
    static void stop_all_channels();

    /**
     * @brief When slog is "stopped", we may still install a channel to log to
     * the console
     */
    static void setup_stopped_channel();

  private:
    Logger();

    ~Logger();

    static Logger& instance();

    /**
     * @brief Signal handler to ensure that log messages are all
     * captured when we get stopped.
     */
    static void slog_signal_handler(int signal_id);

    void do_setup_stopped_channel();

    static std::shared_ptr<LogRecordPool> make_default_pool();

    std::vector<LogChannel> backend;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////

inline Logger& Logger::instance()
{
    static Logger s_logger;
    return s_logger;
}

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


/// Signal handler
extern "C" void slog_handle_signal(int);

/// exit handler
extern "C" void slog_handle_exit();