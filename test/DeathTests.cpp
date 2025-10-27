#include "doctest.h"
#include "slog/LogSetup.hpp"
#include <thread>

TEST_CASE("DieDieDie" * doctest::skip())
{
    start_logger(slog::INFO);
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
