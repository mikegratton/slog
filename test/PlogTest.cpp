#include "slog/slog.hpp"
#ifdef SLOG_PRINTF_LOG
#include "doctest.h"
#include "slog/slog.hpp"
#include "InMemorySink.hpp"
#include "slog/LogSetup.hpp"
#include <iostream>

TEST_CASE("Plog")
{
    auto sink = std::make_shared<InMemorySink>();
    slog::LogConfig config;
    config.set_sink(sink);
    config.set_default_threshold(slog::DBUG);
    slog::start_logger(config);
    Plog(DBUG, "Hello");
    Plogt(DBUG, "tag", "Tagged message");
    Plogtc(DBUG, "tag", 0, "Channeled message message");
    slog::stop_logger();
    for (auto r : sink->contents()) {
        std::cout << r << "\n";
    }
}
#endif