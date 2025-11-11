#include <chrono>
#include <locale>
#include <memory>

#include "slog/Signal.hpp"
#include "slog/slog.hpp"

#include "LogConfig.hpp"
#include "doctest.h"
#include "slog/LogChannel.hpp"
#include "slog/LogRecord.hpp"
#include "slog/LogRecordPool.hpp"
#include "slog/LogSetup.hpp"
#include "slog/LogSink.hpp"
#include "slog/LoggerSingleton.hpp"
#include "slog/ThresholdMap.hpp"
#include "slog/slog.hpp"
#include "slog/slogDetail.hpp"
#include <cstring>

using namespace slog;


TEST_CASE("RecordPool.Discard")
{
    int pool_size = 1024;
    int message_size = 32;    
    LogRecordPool pool(DISCARD, pool_size, message_size);
    CHECK(pool.count() == pool_size / (message_size + sizeof(RecordNode)));
    auto* item = pool.allocate();
    CHECK(item != nullptr);
    pool.free(item);
    auto* item2 = pool.allocate();
    CHECK(item2 != nullptr);
    CHECK(item == item2);
    pool.free(item);

    int total_message = pool.count();
    for (int i = 0; i < total_message ; i++) {
        CHECK(pool.allocate() != nullptr);
    }
    CHECK(pool.allocate() == nullptr);
}


TEST_CASE("RecordPool.Block")
{
    int pool_size = 1024;
    int message_size = 32;
    long blocking_ms = 100;
    LogRecordPool pool(BLOCK, 1024, 32, blocking_ms);
    int total_message = pool.count();
    for (int i = 0; i < total_message ; i++) {
        CHECK(pool.allocate() != nullptr);
    }
    auto t1 = std::chrono::steady_clock::now();
    CHECK(pool.allocate() == nullptr);
    auto t2 = std::chrono::steady_clock::now();
    CHECK(t2 - t1 > std::chrono::milliseconds(blocking_ms));
}


TEST_CASE("RecordPool.Allocate")
{
    int pool_size = 1024;
    int message_size = 32;
    LogRecordPool pool(slog::ALLOCATE, 1024, 32);
    int total_message = pool.count();
    std::vector<RecordNode*> allocated;
    for (int i = 0; i < total_message ; i++) {
        allocated.push_back(pool.allocate());
        CHECK(allocated.back() != nullptr);
    }
    allocated.push_back(pool.allocate());
    CHECK(allocated.back() != nullptr);

    for (auto r : allocated) { pool.free(r); }
    CHECK(pool.count() == 2*total_message);
}

namespace {
    struct TestSink : public slog::LogSink
    {
        int finalized = 0;
        void record(LogRecord const& ) override { };
        void finalize() override { finalized = 1; }
    };
}

TEST_CASE("LogChannel")
{
    ThresholdMap map;
    map.add_tag("foo", slog::NOTE);
    map.set_default(slog::DBUG);
    auto pool = std::make_shared<LogRecordPool>(ALLOCATE, 1024 * 1024, 1024);
    auto sink = std::make_shared<TestSink>();
    LogChannel c(sink, map, pool);
                 
    CHECK(c.threshold("bar") == slog::DBUG);
    CHECK(c.threshold("foo") == slog::NOTE);
    CHECK(c.allocator_count() == pool->count());
    auto r = c.get_fresh_record();
    pool->free(r);
    CHECK(r == c.get_fresh_record());

    CHECK(sink->finalized == 0);
    slog::set_signal_state(SLOG_ACTIVE);
    c.start();
    c.stop();
    CHECK(sink->finalized == 1);
}

TEST_CASE("LoggerSingleton")
{
    using Logger = slog::detail::Logger;
    std::vector<LogConfig> config;
    LogConfig cfg;        
    cfg.set_default_threshold(slog::DBUG);
    config.push_back(cfg);
    cfg.set_default_threshold(slog::WARN);
    config.push_back(cfg);
    Logger::setup_channels(config);
    CHECK(Logger::channel_count() == 2);
    CHECK(Logger::get_channel(1).threshold("") == slog::WARN);
    CHECK(Logger::get_channel(0).threshold("") == slog::DBUG);
    CHECK(Logger::get_channel(100).threshold("") == slog::DBUG);
}

TEST_CASE("LogRecord")
{
    LogRecordMetadata meta;
    meta.capture("filename", "function", 1, 2, "tag");
    CHECK(strcmp(meta.filename, "filename") == 0);
    CHECK(strcmp(meta.function, "function") == 0);
    CHECK(meta.line == 1);
    CHECK(meta.severity == 2);
    CHECK(strcmp(meta.tag , "tag") == 0);

    meta.reset();
    CHECK(strcmp(meta.filename, "") == 0);
    CHECK(strcmp(meta.function, "") == 0);
    CHECK(meta.line == -1);
    CHECK(meta.severity > slog::DBUG);
    for (int i = 0 ; i < slog::TAG_SIZE; i++) { CHECK(meta.tag[i] == 0); }    
}

TEST_CASE("Locale")
{
    ThresholdMap map;
    map.add_tag("foo", slog::NOTE);
    map.set_default(slog::DBUG);
    auto pool = std::make_shared<LogRecordPool>(ALLOCATE, 1024 * 1024, 1024);
    auto sink = std::make_shared<TestSink>();
    LogChannel c(sink, map, pool);

    std::locale loc("en_US.UTF-8");
    slog::set_locale(loc);
    auto* record = slog::get_fresh_record(0, __FILE__, __FUNCTION__, __LINE__, slog::INFO, "foo");
    CHECK(slog::CaptureStream(record, 0).stream().getloc().name() == "en_US.UTF-8");
}

TEST_CASE("LogConfig")
{
    LogConfig cfg;
    cfg.set_default_threshold(slog::DBUG);
    CHECK(cfg.get_threshold_map()["boo"] == slog::DBUG);
    cfg.add_tag("boo", slog::NOTE);
    CHECK(cfg.get_threshold_map()["boo"] == slog::NOTE);
    cfg.set_sink(nullptr);
    CHECK(cfg.get_sink() == nullptr);
    auto pool = std::make_shared<LogRecordPool>(ALLOCATE, 1024, 32);
    cfg.set_pool(pool);
    CHECK(cfg.get_pool() == pool);
}

struct TestRecord : public slog::LogRecord
{
    TestRecord(char* buffer, long length) : slog::LogRecord(buffer, length) {}
};

TEST_CASE("Stream")
{
    LogConfig conifg;
    conifg.set_sink(std::make_shared<NullSink>());    
    int pool_size = 8;
    int message_size = 1024;
    auto pool = std::make_shared<LogRecordPool>(slog::ALLOCATE, pool_size, message_size);
    conifg.set_pool(pool);
    slog::start_logger(conifg);
    auto* record = pool->allocate();
    {
        slog::CaptureStream testStream(record, 0);
        testStream.stream().put('a');
        testStream.stream().put('b');
        CHECK(record->rec.message[0] == 'a');
        CHECK(record->rec.message[1] == 'b');
        testStream.stream().write("hello", 5);
        CHECK(strncmp(record->rec.message, "abhello", 5) == 0);
    }

    // We need to stop things from crashing due to submittal
    slog::stop_logger();
}