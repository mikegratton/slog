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

std::array<LogChannel, SLOG_LOG_MAX_CHANNEL> s_logger;

/**
 * Get a ref to the requested channel, doing a little
 * remapping to make things safe.
 */
LogChannel& get_channel(int channel) {
    if (channel < 0 || channel >= SLOG_LOG_MAX_CHANNEL) {
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

} // end of anon namespace

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// Interface used by macros:

bool will_log(int severity, char const* tag, int channel) {
    return severity <= get_channel(channel).threshold(tag);
}

void push_to_sink(LogRecord* rec) {
    get_channel(rec->meta.channel).push(rec);
}

LogRecord* get_fresh_record(int channel, char const* file, char const* function, int line,
                           int severity, char const* tag) {
    LogRecord* rec = get_channel(channel).allocate();
    if (rec) {
        rec->meta.capture(file, function, line, severity, channel, tag);
    }
    return rec;
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
    auto& channel = get_channel(DEFAULT_CHANNEL);
    channel.set_sink(config.take_sink());
    channel.set_threshold(config.get_threshold_map());
    start_all_channels();
}

void start_logger(std::vector<LogConfig>& config) {
    assert(config.size() <= SLOG_LOG_MAX_CHANNEL);
    unsigned long extent = std::min<unsigned long>(config.size(), SLOG_LOG_MAX_CHANNEL);
    for (unsigned long i=0; i<extent; i++) {
        auto& chan = get_channel(i);
        chan.set_sink(config[i].take_sink());
        chan.set_threshold(config[i].get_threshold_map());
    }
    start_all_channels();
}

void stop_logger() {
    stop_all_channels();
}

void LogConfig::set_sink(std::unique_ptr<LogSink> sink_) {
    if (sink_) {
        sink = std::move(sink_);
    } else {
        sink = std::unique_ptr<LogSink>(new NullSink);
    }
}

}
