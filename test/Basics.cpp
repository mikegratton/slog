#include "doctest.h"
#include "clog.hpp"
#include "LogSetup.hpp"
#include "LogRecordPool.hpp"
#include "LogQueue.hpp"
#include "ThresholdMap.hpp"

#include <thread>
#include <iostream>

using namespace slog;

TEST_CASE("Allocate") {
    LogRecordPool allocator(1024, 32);
    auto* item = allocator.allocate();
    REQUIRE(item != nullptr);
    allocator.deallocate(item);
    auto* item2 = allocator.allocate();
    REQUIRE(item2 != nullptr);
    REQUIRE(item == item2);
}

TEST_CASE("ThresholdMap") {
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

TEST_CASE("Startup") {
    Logf(ERRR, "This is before startup");
    start_logger(INFO);
    Logf(DBUG, "Can't see this");
    Logf(INFO, "Can see this");
    Logf(ERRR, "This is bad");
    Logf(NOTE, "This is note");
    Logf(WARN, "This is warn");
    stop_logger();
    Logf(ERRR, "This is after shutdown");

}

TEST_CASE("DieDieDie" * doctest::skip()) {    
    start_logger(INFO);
    Logf(INFO, "Can see this");
    Logf(FATL, "This is fatal");
    Logf(NOTE, "This is note");    
}

TEST_CASE("Escape" * doctest::skip()) {
    start_logger(INFO);
    for(;;) {
        Logf(INFO, "Hello");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


TEST_CASE("Bomb" * doctest::skip()) {
    start_logger(INFO);
    std::thread t([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        char* p = nullptr;
        *p = 'p';
    });
    for(;;) {
        Logf(INFO, "Hello");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
