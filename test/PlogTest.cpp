#include "slog/LogRecordPool.hpp"
#include "slog/slog.hpp"
#ifdef SLOG_PRINTF_LOG
#include "doctest.h"
#include "slog/slog.hpp"
#include "InMemorySink.hpp"
#include "slog/LogSetup.hpp"

TEST_CASE("Plog")
{
    auto sink = std::make_shared<InMemorySink>();
    auto sink2 = std::make_shared<InMemorySink>();
    std::vector<slog::LogConfig> configs;
    configs.emplace_back();    
    configs.back().set_sink(sink);
    configs.back().set_default_threshold(slog::DBUG);
    configs.emplace_back();    
    configs.back().set_sink(sink2);
    configs.back().set_default_threshold(slog::DBUG);

    slog::start_logger(configs);
    Plog(DBUG, "Hello");
    Plogt(DBUG, "tag", "Tagged message");
    Plogtc(DBUG, "tag", 1, "Channeled message");
    slog::stop_logger();
    
    CHECK(sink->contents()[0] == "Hello");
    CHECK(sink->tags()[0] == "");
    CHECK(sink->contents()[1] == "Tagged message");
    CHECK(sink->tags()[1] == "tag");
    CHECK(sink2->contents()[0] == "Channeled message");    
    CHECK(sink2->tags()[0] == "tag");
}

TEST_CASE("Plog.truncate")
{
    int pool_size = 1024;
    int message_size = 16;
    auto pool = std::make_shared<slog::LogRecordPool>(slog::DISCARD, pool_size, message_size);
    auto sink = std::make_shared<InMemorySink>();
    slog::LogConfig config;
    config.set_sink(sink);
    config.set_pool(pool);
    config.set_default_threshold(slog::DBUG);

    slog::start_logger(config);
    Plog(INFO, "0123456789abcdefgh");
    slog::stop_logger();
    CHECK(sink->contents()[0].size() == 15);
    CHECK(sink->contents()[0] == "0123456789abcde");

    auto* rec = pool->allocate();
    CHECK(rec->capacity() == 16);
    pool->free(rec);

}

#endif