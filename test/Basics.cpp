#include <iostream>
#include <thread>

#include "doctest.h"
#include "slog/LogChannel.hpp"
#include "slog/LogRecordPool.hpp"
#include "slog/LogSetup.hpp"
#include "slog/ThresholdMap.hpp"
#include "slog/slog.hpp"

using namespace slog;

TEST_CASE("RecordPool")
{
    LogRecordPool pool(DISCARD, 1024, 32);
    auto* item = pool.allocate();
    REQUIRE(item != nullptr);
    pool.free(item);
    auto* item2 = pool.allocate();
    REQUIRE(item2 != nullptr);
    REQUIRE(item == item2);
}

TEST_CASE("channelctor")
{
    LogChannel c(std::make_shared<NullSink>(), ThresholdMap(),
                 std::make_shared<LogRecordPool>(ALLOCATE, 1024 * 1024, 1024));
}

TEST_CASE("ThresholdMap")
{
    FlatThresholdMap map;
    map.set_default(INFO);
    map.add_tag("moose", WARN);
    map.add_tag("mongoose", ERRR);
    map.add_tag("mouse", DBUG);
    CHECK(map[""] == INFO);
    CHECK(map["moose"] == WARN);
    CHECK(map["squirrel"] == INFO);
    CHECK(map["mongoose"] == ERRR);
    CHECK(map["mouse"] == DBUG);
    FlatThresholdMap map2 = map;
    CHECK(map2[""] == INFO);
    CHECK(map2["moose"] == WARN);
    CHECK(map2["squirrel"] == INFO);
    CHECK(map2["mongoose"] == ERRR);
    CHECK(map2["mouse"] == DBUG);
}

TEST_CASE("Startup")
{
    Flog(ERRR, "This is before startup");
    start_logger(INFO);
    Flog(DBUG, "Can't see this");
    Flog(INFO, "Can see this");
    Flog(ERRR, "This is bad");
    Flog(NOTE, "This is note");
    Flog(WARN, "This is warn");
    stop_logger();
    Flog(ERRR, "This is after shutdown");
}

TEST_CASE("DieDieDie" * doctest::skip())
{
    start_logger(INFO);
    Flog(INFO, "Can see this");
    Flog(FATL, "This is fatal");
    Flog(NOTE, "This is note");
}

TEST_CASE("Escape" * doctest::skip())
{
    start_logger(slog::INFO);
    for (;;) {
        Flog(INFO, "Hello");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

TEST_CASE("Bomb" * doctest::skip())
{
    start_logger(slog::INFO);
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        char* p = nullptr;
        *p = 'p';
    });
    for (;;) {
        Flog(INFO, "Hello");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
