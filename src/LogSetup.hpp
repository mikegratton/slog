#pragma once
#include <memory>
#include "LogRecord.hpp"
#include "LogSink.hpp"
#include "ThresholdMap.hpp"
#include <vector>

namespace slog {
    
struct LogConfig
{
    LogConfig();
    
    std::unique_ptr<LogSink> sink;
    ThresholdMap threshold;
};

/*
 * Start a logger on the default channel with the FileSink backend default
 * settings and a simple threshold. 
 */
void start_logger(int level = INFO);

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

}
