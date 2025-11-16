#include "LogConfig.hpp"
#include "doctest.h"
#include "slog/slog.hpp"
#include "testUtilities.hpp"
#include <cstring>
#include <thread>

#ifdef SLOG_LOG_TO_CONSOLE_WHEN_STOPPED
TEST_CASE("StoppedLogToConsole")
{
    char const* test_file = "stdout.txt";
    REQUIRE(freopen(test_file, "w", stdout));
    Slog(NOTE, "tag") << "This should be visible";
    // Wait for the logging to finish
    while(slog::free_record_count() != slog::DEFAULT_POOL_RECORD_COUNT) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    REQUIRE(freopen("/dev/tty", "w", stdout));
    FILE* f = fopen(test_file, "r");
    char buffer[1024];
    CHECK(fgets(buffer, sizeof(buffer) - 1, f));
    slog::TestLogRecord record;
    parseLogRecord(record, buffer);
    CHECK(record.meta.severity() == slog::NOTE);
    CHECK(strncmp(record.meta.tag(), "tag", slog::TAG_SIZE - 1) == 0);
    CHECK(strncmp(record.message, "This should be visible", record.message_byte_count) == 0);
    std::remove(test_file);
}
#endif