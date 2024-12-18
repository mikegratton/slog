#include <signal.h>

#include <algorithm>
#include <cassert>
#include <vector>

#include "ConsoleSink.hpp"
#include "FileSink.hpp"
#include "LogChannel.hpp"
#include "LogConfig.hpp"
#include "LogSetup.hpp"

namespace slog {

namespace {

// The global pool is the default when no pool is specified
std::shared_ptr<LogRecordPool> s_global_pool;

// Old signal handlers to be called during shutdown
typedef void (*SignalHandler)(int signum);
SignalHandler g_oldIntHandler = nullptr;
SignalHandler g_oldAbrtHandler = nullptr;
SignalHandler g_oldQuitHandler = nullptr;

////////////////////////////////////////////////////
// This is the actual logger singleton, hidden from
// any other cpp file
class Logger {
   protected:
    std::vector<LogChannel> backend;

    Logger() { do_setup_stopped_channel(); }

    ~Logger() { stop_all_channels(); }

    static Logger& instance()
    {
        static Logger s_logger;
        return s_logger;
    }

    static void ensure_global_pool()
    {
        if (nullptr == s_global_pool) {
            s_global_pool = std::make_shared<LogRecordPool>(ALLOCATE, DEFAULT_POOL_RECORD_COUNT * DEFAULT_RECORD_SIZE,
                                                            DEFAULT_RECORD_SIZE);
        }
    }

   public:
    static std::size_t channel_count() { return instance().backend.size(); }

    static void setup_channels(std::vector<LogConfig> const& config)
    {
        stop_all_channels();
        auto& backend = instance().backend;
        backend.clear();
        ensure_global_pool();
        for (std::size_t i = 0; i < config.size(); i++) {
            std::shared_ptr<LogRecordPool> pool = config[i].get_pool();
            if (nullptr == pool) { pool = s_global_pool; }
            std::shared_ptr<LogSink> sink = config[i].get_sink();
            if (nullptr == sink) { sink = std::make_shared<NullSink>(); }
            backend.emplace_back(sink, config[i].get_threshold_map(), pool);
        }
        // Release the global pool if not in use
        s_global_pool.reset();
    }

    /**
     * Get a ref to the requested channel, doing a little
     * remapping to make things safe.
     */
    static LogChannel& get_channel(int channel)
    {
        if (channel < 0 || channel >= instance().backend.size()) { channel = DEFAULT_CHANNEL; }
        return instance().backend[channel];
    }

    /**
     * Signal handler to ensure that log messages are all
     * captured when we get stopped.
     */
    static void drain_log_queue(int signal_id)
    {
        stop_logger();
        // Pass signal to old handlers
        if (g_oldIntHandler && signal_id == SIGINT) { g_oldIntHandler(signal_id); }
        if (g_oldAbrtHandler && signal_id == SIGABRT) { g_oldAbrtHandler(signal_id); }
        if (g_oldQuitHandler && signal_id == SIGQUIT) { g_oldQuitHandler(signal_id); }
    }

    /**
     * Internal log start function. Also installs thstop_all_channelse
     * signal handler.
     */
    static void start_all_channels()
    {
        for (auto& chan : instance().backend) { chan.start(); }
        struct sigaction action, oldAction;
        sigfillset(&action.sa_mask);
        action.sa_handler = drain_log_queue;
        action.sa_flags = SA_RESTART;

        sigaction(SIGINT, &action, &oldAction);
        g_oldIntHandler = oldAction.sa_handler;
        sigaction(SIGABRT, &action, &oldAction);
        g_oldAbrtHandler = oldAction.sa_handler;
        sigaction(SIGQUIT, &action, &oldAction);
        g_oldQuitHandler = oldAction.sa_handler;
    }

    /**
     * Inverse of start_all_channels
     */
    static void stop_all_channels()
    {
        for (auto& chan : instance().backend) { chan.stop(); }
        struct sigaction action;

        if (g_oldIntHandler) {
            sigfillset(&action.sa_mask);
            action.sa_handler = g_oldIntHandler;
            action.sa_flags = SA_RESTART;
            sigaction(SIGINT, &action, nullptr);
        } else {
            sigaction(SIGINT, nullptr, nullptr);
        }

        if (g_oldAbrtHandler) {
            sigfillset(&action.sa_mask);
            action.sa_handler = g_oldAbrtHandler;
            action.sa_flags = SA_RESTART;
            sigaction(SIGABRT, &action, nullptr);
        } else {
            sigaction(SIGABRT, nullptr, nullptr);
        }

        if (g_oldQuitHandler) {
            sigfillset(&action.sa_mask);
            action.sa_handler = g_oldQuitHandler;
            action.sa_flags = SA_RESTART;
            sigaction(SIGQUIT, &action, nullptr);
        } else {
            sigaction(SIGQUIT, nullptr, nullptr);
        }
    }

    static void setup_stopped_channel() { instance().do_setup_stopped_channel(); }

    void do_setup_stopped_channel()
    {
        ensure_global_pool();
        backend.clear();
        ThresholdMap threshold;
#if SLOG_LOG_TO_CONSOLE_WHEN_STOPPED
        threshold.set_default(DBUG);
        backend.emplace_back(std::make_shared<ConsoleSink>(), threshold, s_global_pool);
#else
        threshold.set_default(FATL);
        backend.emplace_back(std::make_shared<NullSink>(), threshold, s_global_pool);
#endif
        backend.front().start();
    }
};

}  // namespace

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// slog.hpp interface

bool will_log(int severity, char const* tag, int channel)
{
    return severity <= Logger::get_channel(channel).threshold(tag);
}

void push_to_sink(RecordNode* node, int channel)
{
    if (node) { Logger::get_channel(channel).push(node); }
}

RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line, int severity,
                             char const* tag)
{
    RecordNode* node = Logger::get_channel(channel).get_fresh_record();
    if (node) { node->rec.meta.capture(file, function, line, severity, tag); }
    return node;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// LogSetup.hpp interface

void start_logger(int severity)
{
    LogConfig config;
    config.set_default_threshold(severity);
    config.set_sink(std::make_shared<FileSink>());
    start_logger(config);
}

void start_logger(LogConfig const& config)
{
    std::vector<LogConfig> vconfig;
    vconfig.push_back(config);
    start_logger(vconfig);
}

void start_logger(std::vector<LogConfig> const& config)
{
    Logger::stop_all_channels();
    if (config.empty()) { return; }
    Logger::setup_channels(config);
#ifndef SLOG_NO_STREAM
    if (config.size() > 0) { set_locale(config.front().get_locale()); }
#endif
    Logger::start_all_channels();
}

void stop_logger()
{
    Logger::stop_all_channels();
    Logger::setup_stopped_channel();
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

LogConfig::LogConfig() : pool(nullptr), sink(std::make_shared<ConsoleSink>())
{
}

}  // namespace slog
