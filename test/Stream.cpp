#include "doctest.h"
#include "slog.hpp"
#include "LogSetup.hpp"
#include <ostream>
#include "ConsoleSink.hpp"

TEST_CASE("Stream") {
    slog::start_logger(slog::INFO);
    Slog(DBUG) <<  "Can't see this";
    Slog(INFO) << "Can see this";
    Slog(ERRR) << "This is bad";
    Slog(NOTE) << "This is note";
    Slog(WARN) << "This is warn";
    slog::stop_logger();
    Slog(DBUG) <<  "Post stop log";
}

TEST_CASE("JumboMessage") {
    slog::LogConfig config;
    config.set_sink(std::make_shared<slog::ConsoleSink>());
    config.set_default_threshold(slog::INFO);
    config.set_pool(std::make_shared<slog::LogRecordPool>(slog::ALLOCATE, 1024*1024, 64));
    slog::start_logger(config);
    int N = 64*8;
    std::string biggun;
    for (int i=0; i<N; i++) {
        biggun.push_back(i%10? '0' : '1');
    }
    Slog(INFO) << biggun;
    
    slog::stop_logger();
}
