#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "InMemorySink.hpp"
#include "slog/slog.hpp"

TEST_CASE("MultiChannel")
{
    std::vector<slog::LogConfig> configs(2);
    auto sink1 = std::make_shared<InMemorySink>();
    sink1->unlock();
    configs[0].set_sink(sink1);
    auto sink2 = std::make_shared<InMemorySink>();
    sink2->unlock();
    configs[1].set_sink(sink1);
    configs[1].set_default_threshold(slog::NOTE);

    slog::start_logger(configs);
    CHECK(slog::is_channel_active(0) == true);
    CHECK(slog::is_channel_active(1) == true);
    CHECK(slog::is_channel_active(-1) == false);
    CHECK(slog::is_channel_active(2) == false);
    
    Slog(INFO, "tag", 0) << "Message to zero";
    Slog(INFO, "tag") << "Message to default";
    Slog(INFO, "tag", 1) << "Ignored message to 1";
    Slog(NOTE, "tag", 1) << "Recorded message to 1";
    slog::stop_logger();

    REQUIRE(sink1->contents().size() == 2);
    CHECK(sink1->contents()[0] == "Message to zero");
    CHECK(sink1->contents()[1] == "Message to default");

    REQUIRE(sink2->contents().size() == 1);
    CHECK(sink2->contents()[0] == "Recorded message to 1");
}