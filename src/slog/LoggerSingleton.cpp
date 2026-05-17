#include "LoggerSingleton.hpp"
#include "LogChannel.hpp"
#include "LogRecordPool.hpp"
#include "LogSetup.hpp"
#include "LogSink.hpp"
#include "PlatformUtilities.hpp"
#include "Signal.hpp"
#include "SlogError.hpp"
#include "slog/LogWorker.hpp"
#include <cstdlib>
#if SLOG_LOG_TO_CONSOLE_WHEN_STOPPED
#include "ConsoleSink.hpp"
#endif
#include <atomic>
#include <csignal>
#include <cstring>
#include <endian.h>
#include <map>
#include <memory>

namespace slog
{

namespace detail
{

static std::shared_ptr<LogChannel> make_channel(std::shared_ptr<LogSink> sink, ThresholdMap const& threshold,
                                                std::shared_ptr<LogRecordPool> pool)
{
    return std::make_shared<LogChannel>(sink, threshold, pool);
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
: num_worker(0)
{
    // setup_initial_channel();
}

std::shared_ptr<LogRecordPool> Logger::make_default_pool()
{
    return std::make_shared<LogRecordPool>(
        ALLOCATE, DEFAULT_POOL_RECORD_COUNT * (DEFAULT_RECORD_SIZE + sizeof(LogRecord)), DEFAULT_RECORD_SIZE);
}

void Logger::setup_channels(std::vector<LogConfig>& config)
{
    instance().stop();
    if (config.empty()) {
        return;
    }

    std::shared_ptr<LogRecordPool> default_pool;
    auto null_sink = std::make_shared<NullSink>();

    // Determine the number of workers
    std::map<int, std::shared_ptr<LogWorker>> worker;
    auto& channel_worker = instance().channel_worker;
    channel_worker.resize(config.size());
    for (std::size_t channelId = 0; channelId < config.size(); channelId++) {
        auto& con = config[channelId];

        if (worker.count(con.get_worker_thread_id()) == 0) {
            worker[con.get_worker_thread_id()] = std::make_shared<LogWorker>();
        }
        std::shared_ptr<LogWorker> this_worker = worker[con.get_worker_thread_id()];

        std::shared_ptr<LogRecordPool> pool = con.get_pool();
        if (!pool) {
            if (!default_pool) {
                 default_pool = make_default_pool();
            }
            pool = default_pool;
        }

        std::shared_ptr<LogSink> sink = con.get_sink();
        if (!sink) {
            sink = null_sink;
        }

        auto channel = make_channel(sink, con.get_threshold_map(), pool);
        this_worker->add_channel(channelId, channel);
        channel_worker[channelId] = this_worker;
    }
    instance().num_worker = (int) worker.size();
}

void Logger::start()
{
    install_slog_handlers();
    reset_worker_counts();
    set_signal_state(SLOG_ACTIVE);
    for (auto& worker : instance().channel_worker) {
        if (worker) {
            worker->start();
        }
    }
}

void Logger::stop()
{
    set_signal_state(SLOG_STOPPED);
    instance().channel_worker.clear();
    restore_old_signal_handlers();
    s_installed_signal_handlers = false;
    instance().num_worker = 0;
    reset_worker_counts();
}

void Logger::setup_default_channel()
{
    stop();
    ThresholdMap threshold;
    std::shared_ptr<LogRecordPool> pool;
    std::shared_ptr<LogSink> sink;    
#if SLOG_LOG_TO_CONSOLE_WHEN_STOPPED
    threshold.set_default(slog::DBUG);
    pool = make_default_pool();
    sink = std::make_shared<ConsoleSink>();
#else
    threshold.set_default(std::numeric_limits<int>::min());
    pool = std::make_shared<LogRecordPool>(DISCARD, 0, 0);
    sink = std::make_shared<NullSink>();
#endif
    channel_worker.emplace_back(std::make_shared<LogWorker>());
    channel_worker.back()->add_channel(DEFAULT_CHANNEL, make_channel(sink, threshold, pool));
    num_worker = 1;
    start();
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

int Logger::worker_count()
{
    return (int) instance().num_worker;
}

int Logger::channel_count() 
{ 
    return (int) instance().channel_worker.size();    
}


} // namespace detail
} // namespace slog
