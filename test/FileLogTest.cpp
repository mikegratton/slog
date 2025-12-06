#include "doctest.h"
#include "slog/FileSink.hpp"
#include "slog/LogSetup.hpp"
#include "slog/LogSink.hpp"
#include "slog/slog.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include "testUtilities.hpp"

TEST_CASE("FileLog.basic")
{
    slog::LogConfig config;
    auto sink = std::make_shared<slog::FileSink>();
    sink->set_echo(false);        

    CHECK( strncmp(sink->get_file_name(), "./slogTest_", 11) == 0);
    CHECK( strncmp(sink->get_file_name() + 30, ".log", 4) == 0);

    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);
    config.add_tag("ignore", slog::FATL);

    slog::start_logger(config);
    Slog(DBUG) << "invisible";
    Slog(NOTE, "ignore") << "invisible";
    Slog(INFO, "01234567890123456789") << "hello";
    Slog(NOTE) << "goodbye";    
    slog::stop_logger();
    auto time = std::chrono::system_clock::now().time_since_epoch().count();

    // Read the results
    char const* logname = sink->get_file_name();    
    FILE* f = fopen(logname, "r");
    REQUIRE(f);

    // The records
    char buffer[1024];
    slog::TestLogRecord record;
    CHECK(fgets(buffer, sizeof(buffer), f));
    parse_log_record(record, buffer);
    CHECK(record.meta.severity() == slog::INFO);
    CHECK(record.meta.time() < time);
    CHECK(record.meta.time() > time - 100000000UL);
    CHECK(strcmp(record.meta.tag(), "012345678901234") == 0);
    CHECK(strcmp(record.message, "hello\n") == 0);

    CHECK(fgets(buffer, sizeof(buffer), f));
    parse_log_record(record, buffer);
    CHECK(record.meta.severity() == slog::NOTE);
    CHECK(record.meta.time() < time);
    CHECK(record.meta.time() > time - 100000000UL);
    CHECK(strcmp(record.meta.tag(), "") == 0);
    CHECK(strcmp(record.message, "goodbye\n") == 0);

    int count = fscanf(f, "%s", buffer);
    CHECK(count <= 0);    
    CHECK(feof(f));
    fclose(f);

    // Remove the file
    //std::remove(logname);
}

static uint32_t fancyFurniture(FILE* sink, int sequence, unsigned long time)
{
    return fprintf(sink, "FANCY!\n");
}


TEST_CASE("FileLog.furniture")
{
    slog::LogConfig config;
    auto sink = std::make_shared<slog::FileSink>();
    sink->set_echo(false);
    sink->set_file_footer_format(fancyFurniture);
    sink->set_file_header_format(fancyFurniture);
    sink->set_formatter(slog::no_meta_format);
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);

    slog::start_logger(config);
    Slog(NOTE) << "hello";    
    slog::stop_logger();
    
    // Read the results
    char logname[1024];
    strcpy(logname, sink->get_file_name());    
    char buffer[1024];
    slog::TestLogRecord record;
    FILE* f = fopen(logname, "r");
    REQUIRE(f);

    // The header
    memset(buffer, 0, sizeof(buffer));
    CHECK( 7 == fread(buffer, 1, 7, f));
    CHECK(strncmp(buffer, "FANCY!\n", 7) == 0);

    // The record
    memset(buffer, 0, sizeof(buffer));
    CHECK( 6 == fread(buffer, 1, 6, f));
    CHECK(strncmp(buffer, "hello\n", 6) == 0);

    // The footer
    memset(buffer, 0, sizeof(buffer));
    CHECK(7 == fread(buffer, 1, 7, f));
    CHECK(strncmp(buffer, "FANCY!\n", 7) == 0);
    
    // Check there's nothing else
    CHECK(0 == fread(buffer, 1, sizeof(buffer), f));
    CHECK(feof(f));

    fclose(f);

    // Remove the file
    std::remove(logname);
}

TEST_CASE("FileLog.rotation")
{
    auto sink = std::make_shared<slog::FileSink>();
    sink->set_file(".", "otherName", "stem");
    sink->set_max_file_size(16);
    sink->set_formatter(slog::no_meta_format);
    sink->set_echo(false);

    slog::LogConfig config;
    config.set_default_threshold(slog::DBUG);
    config.set_sink(sink);
    slog::start_logger(config);
    Slog(INFO) << "012345678901234567890";
    std::string firstname(sink->get_file_name());
    Slog(INFO) << "012345678901234567890";
    slog::stop_logger();
    
    auto underscore = firstname.find('_');
    REQUIRE(underscore != std::string::npos);
    CHECK(firstname.substr(0, underscore) == "./otherName");

    underscore = firstname.rfind('_');
    REQUIRE(underscore != std::string::npos);
    int sequence = atoi(firstname.data() + underscore + 1);
    CHECK(sequence == 0);
      
    std::string secondName = firstname.substr(0, underscore) + "_001.stem";
    FILE* f = fopen(secondName.c_str(), "r");
    REQUIRE(f);
    char buffer[1024];
    CHECK(fgets(buffer, sizeof(buffer), f));
    CHECK(strncmp(buffer, "012345678901234567890\n", sizeof(buffer)) == 0);

    std::remove(secondName.c_str());
    std::remove(firstname.c_str());
}
