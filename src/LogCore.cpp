#include "LogRecordPool.hpp"
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

LogChannel& get_channel(int channel) {
    if (channel < 0 || channel >= SLOG_LOG_MAX_CHANNEL) {
        channel = DEFAULT_CHANNEL;
    }
    return s_logger[channel];
}

}

namespace detail {

bool should_log(int level, char const* tag, int channel) {
    return level <= get_channel(channel).threshold(tag);
}

void take_record(LogRecord* rec) {
    get_channel(rec->channel).push(rec);
}

LogRecord* allocate_record(int channel) {
    return get_channel(channel).allocate();
}

void drain_log_queue(int signal_id) {
    stop_logger();
    // Re-raise the signal
    signal(signal_id, SIG_DFL);
    raise(signal_id);
}

void start_all_channels() {
    for (auto & chan : s_logger) {
        chan.start();
    }
    
    // Install signal handler
    signal(SIGABRT, drain_log_queue);
    signal(SIGINT, drain_log_queue);
    signal(SIGQUIT, drain_log_queue);
}

void stop_all_channels() {
    for (auto & chan : s_logger) {
        chan.stop();
    }
}

}

void start_logger(int level) {
    auto filesink = std::unique_ptr<FileSink>(new FileSink);
    auto& channel = get_channel(DEFAULT_CHANNEL);
    channel.set_sink(std::move(filesink));
    ThresholdMap tmap;
    tmap.set_default(level);
    channel.set_threshold(tmap);
    detail::start_all_channels();
}

void start_logger(LogConfig& config) {
    auto& channel = get_channel(DEFAULT_CHANNEL);
    channel.set_sink(std::move(config.sink));
    channel.set_threshold(config.threshold);
    detail::start_all_channels();
}

void start_logger(std::vector<LogConfig>& config) {
    assert(config.size() <= SLOG_LOG_MAX_CHANNEL);
    unsigned long extent = std::min<unsigned long>(config.size(), SLOG_LOG_MAX_CHANNEL);
    for (unsigned long i=0; i<extent; i++) {
        if (config[i].sink) {
            auto& chan = get_channel(i);
            chan.set_sink(std::move(config[i].sink));
            chan.set_threshold(config[i].threshold);
        }
    }
    detail::start_all_channels();
}

void stop_logger() {
    detail::stop_all_channels();
}

LogConfig::LogConfig()
    : sink(new NullSink) { }

}
