#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include "slog/FileSink.hpp"
#include "slog/ConsoleSink.hpp"
#include <cstring>
#include "testUtilities.hpp"

/*
012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901
012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901
*/

TEST_CASE("Jumbo.Record")
{
    int constexpr RECORD_SIZE = 64;
    int Nlong = RECORD_SIZE * 3;
    std::string long_text;
    long_text.reserve(Nlong);
    for (int i=0; i < Nlong; i++) {
        int j = i % 10;
        long_text.push_back('0' + j);
    }
    
    slog::LogConfig config;
    auto pool = std::make_shared<slog::LogRecordPool>(slog::ALLOCATE, 1024 * 1024, RECORD_SIZE);    
    config.set_pool(pool);
    auto sink = std::make_shared<slog::FileSink>();
    sink->set_echo(false);
    sink->set_formatter(slog::no_meta_format);
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);
    slog::start_logger(config);    
    Slog(INFO) << long_text << long_text;
    
    slog::stop_logger();    
    FILE* f = fopen(sink->get_file_name(), "r");
    REQUIRE(f);

    auto N = long_text.size() * 4;
    char* buffer = new char[N];
    fgets(buffer, N, f);
    std::string bstring(buffer);
    CHECK(bstring.back() == '\n');
    bstring.pop_back(); // remove the newline
    std::string expected = long_text + long_text;
    CHECK(bstring.size() == expected.size());
    CHECK(bstring == expected);

    CHECK( 0 == fread(buffer, 1, N, f));
    CHECK(feof(f));
    fclose(f);
    std::remove(sink->get_file_name());

}

TEST_CASE("Jumbo.Pool")
{
    slog::LogConfig config;
    config.set_sink(std::make_shared<slog::ConsoleSink>());
    config.set_default_threshold(slog::INFO);
    auto pool = std::make_shared<slog::LogRecordPool>(slog::ALLOCATE, 1024 * 1024, 64);
    long initial_pool_size = pool->count();
    config.set_pool(pool);
    slog::start_logger(config);
    int N = 64 * 8;
    std::string biggun;
    for (int i = 0; i < N; i++) { biggun += "Part ----------------> " + std::to_string(i) + "\n"; }
    Slog(INFO) << biggun;

    slog::stop_logger();
    REQUIRE(pool->count() == initial_pool_size);
}
