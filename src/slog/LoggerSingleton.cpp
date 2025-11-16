#include "LoggerSingleton.hpp"
#include "PlatformUtilities.hpp"
#include "Signal.hpp"
#include "SlogError.hpp"
#include "LogChannel.hpp"
#include "LogRecordPool.hpp"
#include "LogSetup.hpp"
#include "LogSink.hpp"
#if SLOG_LOG_TO_CONSOLE_WHEN_STOPPED
#include "ConsoleSink.hpp"
#endif
#include <atomic>
#include <csignal>
#include <cstring>
#include <endian.h>
#include <memory>
#include <unistd.h>

namespace slog
{

namespace detail
{

static std::unique_ptr<LogChannel> make_channel(std::shared_ptr<LogSink> sink, ThresholdMap const& threshold,
                                                std::shared_ptr<LogRecordPool> pool)
{
    LogChannel* ptr = new LogChannel(sink, threshold, pool);
    return std::unique_ptr<LogChannel>(ptr);
}

/// Install handlers for signals and exit. Idempotent.
std::atomic<bool> static s_installed_signal_handlers{false};
static bool install_slog_handlers()
{
    if (!s_installed_signal_handlers) {
        install_signal_handlers(slog_handle_signal);
        s_installed_signal_handlers = true;
    }

    static std::atomic<bool> s_installed_at_exit_handler{false};
    if (!s_installed_at_exit_handler) {
        if (0 != std::atexit(slog_handle_exit)) {
            slog_error("Failed to install exit handler-- %s\n", strerror(errno));
        }
        s_installed_at_exit_handler = true;
    }
    return true;
}

Logger& Logger::instance()
{
    static Logger s_logger;
    return s_logger;
}

Logger::Logger() 
{ 
    // We don't set up anything by default. We'll wait for a call to
    // put any channels in place
}

std::shared_ptr<LogRecordPool> Logger::make_default_pool()
{
    return std::make_shared<LogRecordPool>(ALLOCATE, DEFAULT_POOL_RECORD_COUNT * (DEFAULT_RECORD_SIZE + sizeof(LogRecord)),
                                           DEFAULT_RECORD_SIZE);
}

void Logger::setup_channels(std::vector<LogConfig>& config)
{
    auto& backend = instance().backend;
    instance().stop_all_channels();
    backend.clear();
    install_slog_handlers();

    auto default_pool = make_default_pool();
    auto null_sink = std::make_shared<NullSink>();

    for (auto& con : config) {
        std::shared_ptr<LogRecordPool> pool = con.get_pool();
        if (!pool) {
            pool = default_pool;
        }
        std::shared_ptr<LogSink> sink = con.get_sink();
        if (nullptr == sink) {
            sink = null_sink;
        }
        backend.emplace_back(make_channel(sink, con.get_threshold_map(), pool));
    }
}

void Logger::start_all_channels()
{
    auto& logger = Logger::instance();
    set_signal_state(SLOG_ACTIVE);
    for (auto& chan : logger.backend) {
        chan->start();
    }
}

/**
 * Inverse of start_all_channels
 */
void Logger::stop_all_channels()
{
    auto& logger = Logger::instance();
    if (get_signal_state() == SLOG_ACTIVE) {
        set_signal_state(SLOG_STOPPED);
    }
    logger.backend.clear();
    restore_old_signal_handlers();
    s_installed_signal_handlers = false;
}

void Logger::setup_stopped_channel() { instance().do_setup_stopped_channel(); }

void Logger::do_setup_stopped_channel()
{
    if (get_signal_state() == SLOG_ACTIVE) {
        set_signal_state(SLOG_STOPPED);
    }
    backend.clear();
    ThresholdMap threshold;
#if SLOG_LOG_TO_CONSOLE_WHEN_STOPPED
    threshold.set_default(slog::DBUG);
    auto pool = make_default_pool();
    auto sink = std::make_shared<ConsoleSink>();
#else
    threshold.set_default(std::numeric_limits<int>::min());
    auto pool = std::make_shared<LogRecordPool>(DISCARD, 0, 0);
    auto sink = std::make_shared<NullSink>();
#endif
    backend.emplace_back(make_channel(sink, threshold, pool));
    set_signal_state(SLOG_ACTIVE);
    backend.front()->start();
}

long get_pool_missing_count()
{
    long count = 0;
    for (int i = 0; i < Logger::channel_count(); i++) {
        auto& chan = Logger::get_channel(i);
        count += chan.pool_free_count();
    }
    return count;
}

} // namespace detail
} // namespace slog
