#include "LogConfig.hpp"
#include "doctest.h"
#include "slog/FileSink.hpp"
#include "slog/LogRecord.hpp"
#include "slog/slog.hpp"
#include "slog/LogSetup.hpp"
#include "testUtilities.hpp"

#include <cstring>
#include <pthread.h>
#include <unistd.h>

TEST_CASE("Flog")
{
    slog::LogConfig config;
    std::shared_ptr<slog::FileSink> sink = std::make_shared<slog::FileSink>();
    sink->set_file(".", "flogTest");
    sink->set_echo(false);
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);

    Flog(ERRR, "This is before startup");
    slog::start_logger(config);
    Flog(DBUG, "Invisible");
    Flog(INFO, "Info text %d", 7);
    Flog(WARN, "Hello %s", "guv");
    Flogt(INFO, "tag", "Tag message %d", 1);
    Flogtc(INFO, "tag2", 0, "Channel message %d", 2);

    slog::stop_logger();
    Flog(ERRR, "This is after shutdown");
    
    auto time = std::chrono::system_clock::now().time_since_epoch().count();

    FILE* f = fopen(sink->get_file_name(), "r");
    REQUIRE(f);
    char buffer[1024];
    slog::TestLogRecord record;
    fgets(buffer, sizeof(buffer), f);
    slog::parse_log_record(record, buffer);
    CHECK(record.meta.severity() == slog::INFO);
    CHECK(record.meta.tag()[0] == '\0');
    CHECK(record.meta.time() < time);
    CHECK(record.meta.time() > time - 100000000UL);
    CHECK(strcmp(record.message, "Info text 7\n") == 0);

    fgets(buffer, sizeof(buffer), f);
    slog::parse_log_record(record, buffer);
    CHECK(record.meta.severity() == slog::WARN);    
    CHECK(strcmp(record.message, "Hello guv\n") == 0);

    fgets(buffer, sizeof(buffer), f);
    slog::parse_log_record(record, buffer);
    CHECK(record.meta.severity() == slog::INFO);    
    CHECK(strncmp(record.meta.tag(), "tag", 15) == 0);
    CHECK(strcmp(record.message, "Tag message 1\n") == 0);

    fgets(buffer, sizeof(buffer), f);
    slog::parse_log_record(record, buffer);
    CHECK(record.meta.severity() == slog::INFO);    
    CHECK(strncmp(record.meta.tag(), "tag2", 15) == 0);
    CHECK(strcmp(record.message, "Channel message 2\n") == 0);

    fclose(f);
    unlink(sink->get_file_name());
}
