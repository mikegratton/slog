#include "LoggerSingleton.hpp"
#include "PlatformUtilities.hpp"
#include "ConsoleSink.hpp"
#include "slog/SlogError.hpp"

namespace slog
{
namespace detail
{

Logger& Logger::instance()
{
    static Logger s_logger;
    return s_logger;
}

Logger::Logger()    
{
    do_setup_stopped_channel();
}

Logger::~Logger() { stop_all_channels(); }

void Logger::change_run_state(int signal_id)
{
    instance().signal_state = signal_id;
}

int Logger::get_run_state()
{
    return instance().signal_state;
}

std::shared_ptr<LogRecordPool> Logger::make_default_pool()
{
    return std::make_shared<LogRecordPool>(
        ALLOCATE, DEFAULT_POOL_RECORD_COUNT * DEFAULT_RECORD_SIZE,
        DEFAULT_RECORD_SIZE);
}

void Logger::setup_channels(std::vector<LogConfig>& config)
{
    instance().stop_all_channels();
    auto& backend = instance().backend;
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

/**
 * Internal log start function. Also installs stop_all_channels
 * signal handler.
 */
void Logger::start_all_channels()
{
    for (auto& chan : instance().backend) {
        chan.start();
    }

    static std::atomic<bool> s_installedHandlers{false};
    if (!s_installedHandlers) {
        install_signal_handlers(slog_handle_signal);
        s_installedHandlers = true;
    }
    
    static std::atomic<bool> s_installedAtExit{false};
    if (!s_installedAtExit) {
        if( 0 != std::atexit(slog_handle_exit) ) {
            slog_error("Failed to install exit handler\n");            
        }
        s_installedAtExit = true;
    }
}

/**
 * Inverse of start_all_channels
 */
void Logger::stop_all_channels()
{
    for (auto& chan : instance().backend) {
        chan.stop();
    }
}

void Logger::setup_stopped_channel() { instance().do_setup_stopped_channel(); }

void Logger::do_setup_stopped_channel()
{
    restore_old_signal_handlers();
    backend.clear();
    ThresholdMap threshold;
#if SLOG_LOG_TO_CONSOLE_WHEN_STOPPED
    threshold.set_default(DBUG);
    backend.emplace_back(std::make_shared<ConsoleSink>(), threshold,
                         make_default_pool());
#else
    threshold.set_default(FATL);
    backend.emplace_back(std::make_shared<NullSink>(), threshold,
                         make_default_pool());
#endif
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

extern "C" void slog_handle_signal(int signal)
{
    slog::detail::Logger::stop_all_channels();
    slog::forward_signal(signal);
}

extern "C" void slog_handle_exit()
{
    slog::detail::Logger::stop_all_channels();
}