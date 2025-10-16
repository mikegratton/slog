
#include <cassert>
#include <vector>

#include "ConsoleSink.hpp"
#include "FileSink.hpp"
#include "LoggerSingleton.hpp"

namespace slog {

using Logger = ::slog::detail::Logger;


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
