#include "LogConfig.hpp"
#include "LogSetup.hpp"
#include "FileSink.hpp"
#include "LogChannel.hpp"

#include <cassert>
#include <signal.h>
#include <array>
#include <algorithm>

namespace slog {


namespace {

////////////////////////////////////////////////////
// This is the actual logger singleton, hidden from
// any other cpp file
class Logger {
protected:
    std::array<LogChannel, SLOG_MAX_CHANNEL> backend;
    LogRecordPool pool;
    
    Logger() : pool(SLOG_POOL_SIZE, SLOG_MESSAGE_SIZE)
    {
        for (int i=0; i<SLOG_MAX_CHANNEL; i++) {
            backend[i].set_pool(&pool);
        }
    }
    
    static Logger& instance() {
        static Logger s_logger;
        return s_logger;
    }

public:

    /**
     * Get a ref to the requested channel, doing a little
     * remapping to make things safe.
     */
    static LogChannel& get_channel(int channel) {
        if (channel < 0 || channel >= SLOG_MAX_CHANNEL) {
            channel = DEFAULT_CHANNEL;
        }
        return instance().backend[channel];
    }

    /**
    * Signal handler to ensure that log messages are all
    * captured when we get stopped.
    */
    static void drain_log_queue(int signal_id) {
        stop_logger();
        // Re-raise the signal
        signal(signal_id, SIG_DFL);
        raise(signal_id);
    }

    /**
    * Internal log start function. Also installs the
    * signal handler.
    */
    static void start_all_channels() {
        for (auto& chan : instance().backend) {
            chan.start();
        }

        // Install signal handler
        signal(SIGABRT, drain_log_queue);
        signal(SIGINT, drain_log_queue);
        signal(SIGQUIT, drain_log_queue);
    }

    /**
     * Inverse of start_all_channels
     */
    static void stop_all_channels() {
        for (auto& chan : instance().backend) {
            chan.stop();
        }
        signal(SIGABRT, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
    }

    static bool setup_console_channel() {
#if SLOG_DUMP_TO_CONSOLE_WHEN_STOPPED
        auto& chan = instance().backend[DEFAULT_CHANNEL];
        chan.stop();
        chan.set_sink(std::unique_ptr<ConsoleSink>(new ConsoleSink));
        ThresholdMap threshold;
        threshold.set_default(DBUG);
        chan.set_threshold(threshold);
        chan.start();
        return true;
#else
        return false;
#endif
    }
};

// This forces this to be run before main
const bool s_DEFAULT_TO_CONSOLE = Logger::setup_console_channel();

} // end of anon namespace

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// slog.hpp interface

bool will_log(int severity, char const* tag, int channel) {
    return severity <= Logger::get_channel(channel).threshold(tag);
}

void push_to_sink(RecordNode* node, int channel) {
    if (node) {
        Logger::get_channel(channel).push(node);
    }
}

RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line,
                             int severity, char const* tag) {
    RecordNode* node = Logger::get_channel(channel).get_fresh_record();
    if (node) {
        node->rec.meta.capture(file, function, line, severity, tag);
    }
    return node;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// LogSetup.hpp interface

void start_logger(int severity) {
    LogConfig config;
    config.set_default_threshold(severity);
    config.set_sink(new FileSink);
    start_logger(config);
}

void start_logger(LogConfig& config) {
    Logger::stop_all_channels();
    auto& channel = Logger::get_channel(DEFAULT_CHANNEL);
    channel.set_sink(config.take_sink());
    channel.set_threshold(config.get_threshold_map());
    Logger::start_all_channels();
}

void start_logger(std::vector<LogConfig>& config) {
    Logger::stop_all_channels();
    assert(config.size() <= SLOG_MAX_CHANNEL);
    unsigned long extent = std::min<unsigned long>(config.size(), SLOG_MAX_CHANNEL);
    for (unsigned long i=0; i<extent; i++) {
        auto& chan = Logger::get_channel(i);
        chan.set_sink(config[i].take_sink());
        chan.set_threshold(config[i].get_threshold_map());
    }
    Logger::start_all_channels();
}

void stop_logger() {
    Logger::stop_all_channels();
    Logger::setup_console_channel();
}

void LogConfig::set_sink(std::unique_ptr<LogSink> sink_) {
    if (sink_) {
        sink = std::move(sink_);
    } else {
        sink = std::unique_ptr<LogSink>(new NullSink);
    }
}

long get_pool_missing_count() {
    long count = 0;
    for (unsigned long i=0; i<SLOG_MAX_CHANNEL; i++) {
        auto& chan = Logger::get_channel(i);
        count += chan.allocator_count();
    }
    return count;
}

}

