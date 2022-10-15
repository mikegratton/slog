#pragma once
#include <memory>
#include "LogRecord.hpp"
#include "LogSink.hpp"
#include "ThresholdMap.hpp"
#include <vector>

/**
 * This is only needed where you are starting/stopping the logger.
 * 
 * Quick version:
 * 
 * ```
 * #include <LogSetup.hpp>
 * #include <slog.hpp> // You must include slog.hpp or clog.hpp to use the logging macros
 * 
 * int main(int argc, char** argv) {
 *    
 *    Log(WARN) << "No one will see this message.";
 *    
 *    slog::start_logger(NOTE);
 *    Log(NOTE) << "This is logged using the FileSink with the default setup";
 *    Log(INFO) << "This isn't logged because INFO < NOTE";
 * 
 *    slog::stop_logger(); // This is not required, but will stop all messages from printing
 *    Log(WARN) << "No one will see this message.";
 *    
 *    return 0;
 * }
 * ```
 */

namespace slog {
    
class LogConfig;

/*
 * Start a logger on the default channel with the FileSink backend default
 * settings and a simple threshold. 
 */
void start_logger(int severity = INFO);

/*
 * Start a logger on the default channel with the given config.
 */
void start_logger(LogConfig& config);

/*
 * Start loggers on channels [0, configs.size()) with the 
 * given configs.
 */
void start_logger(std::vector<LogConfig>& configs);

/*
 * Stop all channels, draining the queue into the sinks.
 * Note: this prevents further messages from being logged.
 */
void stop_logger();

// For debug
long get_pool_missing_count();

/**
 * Configuration object for a logger channel. Setup your logging threshold, 
 * tags, and desired log sink here. By default, the sink is a NullSink that
 * discards all messages.
 */
class LogConfig
{
public:
    LogConfig() : sink(new NullSink) { }
    
    void set_default_threshold(int thr) { threshold.set_default(thr); }

    void add_tag(const char* tag, int thr) { threshold.add_tag(tag, thr); }
    
    void set_sink(std::unique_ptr<LogSink> sink_);
    
    /**
     * Takes ownership of the sink pointer.
     */
    void set_sink(LogSink* sink_) { set_sink(std::unique_ptr<LogSink>(sink_)); }
    
    
    std::unique_ptr<LogSink> take_sink() { return std::move(sink); }
    
    ThresholdMap const& get_threshold_map() const { return threshold; }
    
protected:
    std::unique_ptr<LogSink> sink;
    ThresholdMap threshold;
};

}
