#include "doctest.h"
#include "slog/JournaldSink.hpp"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"

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
