#include "doctest.h"
#include "slog/Timestamp.hpp"
#include <string>

TEST_CASE("Timestamp")
{
    slog::Timestamp old(10);
    slog::Timestamp now = slog::Timestamp::now();
    CHECK(old.nanoseconds_since_epoch() == 10ul);
    CHECK(now.nanoseconds_since_epoch() > old.nanoseconds_since_epoch());

    char buf[32];
    old.format_time(buf);
    CHECK(std::string("1970-01-01 00:00:00.000Z") == buf);

    old.format_time(buf, 0);
    CHECK(std::string("1970-01-01 00:00:00Z") == buf);

    old.format_time(buf, 0, slog::Timestamp::COMPACT);
    CHECK(std::string("19700101T000000Z") == buf);

    old.format_time(buf, 0, slog::Timestamp::FULL_T);
    CHECK(std::string("1970-01-01T00:00:00Z") == buf);
}