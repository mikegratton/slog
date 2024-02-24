#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "slog/SyslogSink.hpp"

using namespace slog;

TEST_CASE("SyslogTest")
{
    auto sink = std::make_shared<slog::SyslogSink>("/dev/log");
    sink->set_rfc3164_protocol(true);
    slog::LogConfig config;
    config.set_default_threshold(slog::DBUG);
    config.set_sink(sink);
    slog::start_logger(config);
    Slog(NOTE) << "Testing!";
    Slog(WARN) << "Warning!";
}
