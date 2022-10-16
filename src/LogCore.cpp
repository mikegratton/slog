#include "LogConfig.hpp"
#include "LogSetup.hpp"
#include "FileSink.hpp"
#include "LogChannel.hpp"

#include <cassert>
#include <signal.h>
#include <array>
#include <algorithm>

namespace slog {

//////////////////////////////////////////////
// This is the actual logger "singleton" --
// a global variable at cpp-file scope here.
// It's an array indexed by channel
namespace {

std::array<LogChannel, SLOG_MAX_CHANNEL> s_logger;

/**
 * Get a ref to the requested channel, doing a little
 * remapping to make things safe.
 */
LogChannel& get_channel(int channel) {
    if (channel < 0 || channel >= SLOG_MAX_CHANNEL) {
        channel = DEFAULT_CHANNEL;
    }
    return s_logger[channel];
}

/**
 * Signal handler to ensure that log messages are all
 * captured when we get stopped.
 */
void drain_log_queue(int signal_id) {
    stop_logger();
    // Re-raise the signal
    signal(signal_id, SIG_DFL);
    raise(signal_id);
}

/**
 * Internal log start function. Also installs the
 * signal handler.
 */
void start_all_channels() {
    for (auto& chan : s_logger) {
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
void stop_all_channels() {
    for (auto& chan : s_logger) {
       chan.stop();
    }
    signal(SIGABRT, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

bool setup_console_channel() {    
#ifdef SLOG_DUMP_TO_CONSOLE_WHEN_STOPPED
    auto& chan = s_logger[DEFAULT_CHANNEL];
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

// This forces this to be run before main
const bool s_DEFAULT_TO_CONSOLE = setup_console_channel();

} // end of anon namespace

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// Interface used by macros:

bool will_log(int severity, char const* tag, int channel) {
    return severity <= get_channel(channel).threshold(tag);
}

void push_to_sink(RecordNode* node) {
    if (node) {
        get_channel(node->channel).push(node);
    }
}

RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line,
                           int severity, char const* tag) {
    RecordNode* node = get_channel(channel).allocate();    
    if (node) {
        node->channel = channel;
        node->rec.meta.capture(file, function, line, severity, tag);
    }
    return node;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// LogSetup Interface

void start_logger(int severity) {
    LogConfig config;
    config.set_default_threshold(severity);
    config.set_sink(new FileSink);
    start_logger(config);
}

void start_logger(LogConfig& config) {
    stop_all_channels();
    auto& channel = get_channel(DEFAULT_CHANNEL);
    channel.set_sink(config.take_sink());
    channel.set_threshold(config.get_threshold_map());
    start_all_channels();
}

void start_logger(std::vector<LogConfig>& config) {
    stop_all_channels();
    assert(config.size() <= SLOG_MAX_CHANNEL);
    unsigned long extent = std::min<unsigned long>(config.size(), SLOG_MAX_CHANNEL);
    for (unsigned long i=0; i<extent; i++) {
        auto& chan = get_channel(i);
        chan.set_sink(config[i].take_sink());
        chan.set_threshold(config[i].get_threshold_map());
    }
    start_all_channels();
}

void stop_logger() {
    stop_all_channels();
    setup_console_channel();
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
        auto& chan = get_channel(i);
        count += chan.allocator_count();
    }
    return count;
}

}
