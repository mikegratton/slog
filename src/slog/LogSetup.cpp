
#include "LogSetup.hpp"
#include <cassert>
#include <vector>

#include "ConsoleSink.hpp"
#include "LoggerSingleton.hpp"

namespace slog
{

using Logger = ::slog::detail::Logger;

void start_logger(LogConfig const& config)
{
    std::vector<LogConfig> vconfig{config};
    start_logger(vconfig);
}

void start_logger(std::vector<LogConfig> config)
{
    if (config.empty()) {
        return;
    }
    Logger::setup_channels(config);
#if SLOG_STREAM
    if (config.size() > 0) {
        set_locale(config.front().get_locale());
    }
#endif
    Logger::start_all_channels();
}

LogConfig::LogConfig()
    : pool(nullptr),
      sink(std::make_shared<ConsoleSink>())
{
}

LogConfig::LogConfig(int default_threshold, std::shared_ptr<LogSink> new_sink)
    : sink(new_sink)
{
    threshold.set_default(default_threshold);
}

} // namespace slog
