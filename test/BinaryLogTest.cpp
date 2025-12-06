#include "doctest.h"
#include "slog/BinarySink.hpp"
#include "slog/LogRecordPool.hpp"
#include "slog/LogSetup.hpp"
#include "slog/LogSink.hpp"
#include "slog/slog.hpp"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>

#if SLOG_BINARY_LOG

TEST_CASE("BinaryishLog")
{
    slog::LogConfig config;
    std::shared_ptr<slog::BinarySink> sink = std::make_shared<slog::BinarySink>();
    sink->set_file(".", "blogTest");
    sink->set_formatter(slog::long_binary_format);
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);

    slog::start_logger(config);
    Blog(INFO).record("hello!", 7);
    Blog(INFO, "tag").record("tagged", 7);
    Blog(INFO, "moof", 0).record("moof", 5);
    Blog(NOTE, "fizzbuzz", 0).record("moof", 5);
    slog::stop_logger();
    auto time = std::chrono::system_clock::now().time_since_epoch().count();

    // Read the results
    char const* logname = sink->get_file_name();
    char buffer[1024];
    FILE* f = fopen(logname, "r");
    REQUIRE(f);

    // The header
    auto count = fread(buffer, 1, 8, f);
    CHECK(strncmp(buffer, "SLOG", 4) == 0);
    CHECK((unsigned char) buffer[4] == 0xff);
    CHECK((unsigned char) buffer[5] == 0xfe);
    CHECK(buffer[6] == 0);
    CHECK(buffer[7] == 0);

    // The records
    CHECK(fread(buffer, 1, 32, f));
    CHECK(*reinterpret_cast<uint32_t*>(buffer) == 7);
    CHECK(*reinterpret_cast<int32_t*>(buffer + 4) == 600);    
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) > time - std::chrono::nanoseconds(100000000).count());
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) < time);
    CHECK(strncmp(buffer + 16, "", 16) == 0);
    CHECK(fread(buffer, 1, 7, f));
    CHECK(strncmp(buffer, "hello!", 7) == 0);

    CHECK(fread(buffer, 1, 32, f));
    CHECK(*reinterpret_cast<uint32_t*>(buffer) == 7);
    CHECK(*reinterpret_cast<int32_t*>(buffer + 4) == 600);    
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) > time - std::chrono::nanoseconds(100000000).count());
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) < time);
    CHECK(strncmp(buffer + 16, "tag", 16) == 0);
    CHECK(fread(buffer, 1, 7, f));
    CHECK(strncmp(buffer, "tagged", 7) == 0);

    CHECK(fread(buffer, 1, 32, f));
    CHECK(*reinterpret_cast<uint32_t*>(buffer) == 5);
    CHECK(*reinterpret_cast<int32_t*>(buffer + 4) == 600);    
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) > time - std::chrono::nanoseconds(100000000).count());
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) < time);
    CHECK(strncmp(buffer + 16, "moof", 16) == 0);
    CHECK(fread(buffer, 1, 5, f));
    CHECK(strncmp(buffer, "moof", 5) == 0);

    CHECK(fread(buffer, 1, 32, f));
    CHECK(*reinterpret_cast<uint32_t*>(buffer) == 5);
    CHECK(*reinterpret_cast<int32_t*>(buffer + 4) == 500);    
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) > time - std::chrono::nanoseconds(100000000).count());
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) < time);
    CHECK(strncmp(buffer + 16, "fizzbuzz", 16) == 0);
    CHECK(fread(buffer, 1, 5, f));
    CHECK(strncmp(buffer, "moof", 5) == 0);

    fclose(f);

    // Remove the file
    std::remove(logname);    
}

static uint32_t fancyFurniture(FILE* sink, int sequence, uint64_t time)
{
    return fprintf(sink, "FANCY!");
}

TEST_CASE("BinaryLog")
{
    slog::stop_logger();
    slog::LogConfig config;
    auto sink = std::make_shared<slog::BinarySink>();
    sink->set_file(".", "testBinaryLog", "blog");
    sink->set_max_file_size(40);
    sink->set_file_header_format(fancyFurniture);
    sink->set_file_footer_format(fancyFurniture);
    sink->set_formatter(slog::default_binary_format);
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);
    slog::start_logger(config);
    long bits = 0xfffff;
    Blog(INFO, "tag1").record(&bits, sizeof(bits));
    Blog(INFO, "0123456789abcdef").record(&bits, sizeof(bits));        
    slog::stop_logger();

    FILE* f = fopen(sink->get_file_name(), "rb");
    REQUIRE(f);

    char buffer[1024];
    uint32_t record_size;
    long data;
    std::size_t readItems;

    // Header
    readItems = fread(buffer, 1, 6, f);
    REQUIRE(readItems == 6);
    CHECK(strncmp(buffer, "FANCY!", 6) == 0);

    // Records    
    readItems = fread(&record_size, sizeof(record_size), 1, f);
    REQUIRE(readItems == 1);
    CHECK(record_size == 8);    
    readItems = fread(buffer, 1, 4, f);
    REQUIRE(readItems == 4);
    CHECK(strncmp(buffer, "tag1", 4) == 0);
    readItems = fread(&data, sizeof(long), 1, f);
    REQUIRE(readItems == 1);
    CHECK(data == bits);

    readItems = fread(&record_size, sizeof(record_size), 1, f);
    REQUIRE(readItems == 1);
    CHECK(record_size == 8);
    readItems = fread(buffer, 1, 4, f);
    REQUIRE(readItems == 4);
    CHECK(strncmp(buffer, "0123", 4) == 0);
    readItems = fread(&data, sizeof(long), 1, f);
    REQUIRE(readItems == 1);
    CHECK(data == bits);

    // Footer
    buffer[0] = 0;    
    readItems = fread(buffer, 1, 6, f);    
    REQUIRE(readItems == 6);
    CHECK(strncmp(buffer, "FANCY!", 6) == 0);

    // Check there's no more data
    CHECK( 0 == fread(buffer, 1, 1024, f));
    CHECK(feof(f));

    fclose(f);
    std::remove(sink->get_file_name());
}


TEST_CASE("Binary.jumbo")
{
    slog::LogConfig config;
    std::shared_ptr<slog::BinarySink> sink = std::make_shared<slog::BinarySink>();    
    sink->set_file(".", "blogTest");
    sink->set_formatter(slog::long_binary_format);
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);
    auto pool = std::make_shared<slog::LogRecordPool>(slog::BLOCK, 1024, 32);
    config.set_pool(pool);
    slog::start_logger(config);

    char const* message = "---------1---------1---------1---------1---------1---------1";
    Blog(NOTE, "tag").record(message, strlen(message));    

    slog::stop_logger();
    auto time = std::chrono::system_clock::now().time_since_epoch().count();

    // Read the results
    char const* logname = sink->get_file_name();
    char buffer[1024];
    FILE* f = fopen(logname, "r");
    REQUIRE(f);

    // The header
    auto count = fread(buffer, 1, 8, f);
    CHECK(strncmp(buffer, "SLOG", 4) == 0);
    CHECK((unsigned char) buffer[4] == 0xff);
    CHECK((unsigned char) buffer[5] == 0xfe);
    CHECK(buffer[6] == 0);
    CHECK(buffer[7] == 0);

    // The records
    CHECK(fread(buffer, 1, 32, f));
    CHECK(*reinterpret_cast<uint32_t*>(buffer) == strlen(message));
    CHECK(*reinterpret_cast<int32_t*>(buffer + 4) == slog::NOTE);    
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) > time - std::chrono::nanoseconds(100000000).count());
    CHECK(*reinterpret_cast<uint64_t*>(buffer + 8) < time);
    CHECK(strncmp(buffer + 16, "tag", 16) == 0);
    CHECK(fread(buffer, 1, 60, f));
    CHECK(strncmp(buffer, message, strlen(message)) == 0);

    fclose(f);

    // Remove the file
    std::remove(logname);    
}

#endif