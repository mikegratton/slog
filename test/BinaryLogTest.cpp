#include "doctest.h"
#include "slog/BinarySink.hpp"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"

TEST_CASE("BinaryishLog")
{
    slog::start_logger();
    Blog(INFO).record("hello!", 7);
    Blog(INFO, "tag").record("tagged", 7);
    Blog(INFO, "moof", 0).record("moof", 5);
}

static uint32_t fancyFurniture(FILE* sink, int sequence, unsigned long time)
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
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);
    slog::start_logger(config);
    long bits = 0xfffff;
    Blog(INFO, "tag1").record(&bits, sizeof(bits));
    Blog(INFO, "0123456789abcdef").record(&bits, sizeof(bits));
    Blog(INFO, "tag1").record(&bits, sizeof(bits));

    CHECK(bits > 0);
    slog::stop_logger();
}