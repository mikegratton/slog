#include "LoggerSingleton.hpp"
#include "ConsoleSink.hpp"
#include "PlatformUtilities.hpp"
#include "Signal.hpp"
#include "SlogError.hpp"
#include <atomic>
#include <cstring>
#include <endian.h>
#include <unistd.h>

namespace slog
{

namespace detail
{

/// Install handlers for signals and exit. Idempotent.
std::atomic<bool> static s_installedSignalHandlers{false};
static bool install_slog_handlers()
{
    if (!s_installedSignalHandlers) {
        install_signal_handlers(slog_handle_signal);
        s_installedSignalHandlers = true;
    }

    static std::atomic<bool> s_installedAtExitHandler{false};
    if (!s_installedAtExitHandler) {
        if (0 != std::atexit(slog_handle_exit)) {
            slog_error("Failed to install exit handler-- %s\n", strerror(errno));
        }
        s_installedAtExitHandler = true;
    }
    return true;
}

Logger& Logger::instance()
{
    static Logger s_logger;
    return s_logger;
}

Logger::Logger() { do_setup_stopped_channel(); }

Logger::~Logger() { stop_all_channels(); }

std::shared_ptr<LogRecordPool> Logger::make_default_pool()
{
    return std::make_shared<LogRecordPool>(ALLOCATE, DEFAULT_POOL_RECORD_COUNT * DEFAULT_RECORD_SIZE,
                                           DEFAULT_RECORD_SIZE);
}

void Logger::setup_channels(std::vector<LogConfig>& config)
{    
    install_slog_handlers();

    auto& backend = instance().backend;
    instance().stop_all_channels();
    backend.clear();
    auto default_pool = make_default_pool();
    
    for (std::size_t i = 0; i < config.size(); i++) {
        std::shared_ptr<LogRecordPool> pool = config[i].get_pool();
        if (!pool) {
            pool = default_pool;
        }
        std::shared_ptr<LogSink> sink = config[i].get_sink();
        if (nullptr == sink) {
            sink = std::make_shared<NullSink>();
        }
        backend.emplace_back(sink, config[i].get_threshold_map(), pool);
    }
}

void Logger::start_all_channels()
{
    Logger::instance();
    set_signal_state(SLOG_ACTIVE);
    for (auto& chan : instance().backend) {
        chan.start();
    }
}

/**
 * Inverse of start_all_channels
 */
void Logger::stop_all_channels()
{
    Logger::instance();
    set_signal_state(SLOG_STOPPED);
    for (auto& chan : instance().backend) {
        chan.stop();
    }
    restore_old_signal_handlers();
    s_installedSignalHandlers = false;
}

void Logger::setup_stopped_channel() { instance().do_setup_stopped_channel(); }

void Logger::do_setup_stopped_channel()
{
    set_signal_state(SLOG_STOPPED);
    backend.clear();
    ThresholdMap threshold;
    threshold.set_default(FATL);
    backend.emplace_back(std::make_shared<NullSink>(), threshold, make_default_pool());
    set_signal_state(SLOG_ACTIVE);
    backend.front().start();
}

long get_pool_missing_count()
{
    long count = 0;
    for (unsigned long i = 0; i < Logger::channel_count(); i++) {
        auto& chan = Logger::get_channel(i);
        count += chan.allocator_count();
    }
    return count;
}

} // namespace detail
} // namespace slog
