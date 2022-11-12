#include "doctest.h"
#include "slog.hpp"
#include "JournaldSink.hpp"
#include "LogSetup.hpp"

TEST_CASE("Journald")
{
    slog::LogConfig config;
    config.set_default_threshold(slog::INFO);    
    config.set_sink(std::make_shared<slog::JournaldSink>());
    slog::start_logger(config);
    
    Slog(INFO) << "Hello, journal!";
    Slog(NOTE) << "And at NOTE";
    slog::stop_logger();
}
