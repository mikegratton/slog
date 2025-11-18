#include "doctest.h"
#include "slog/PlatformUtilities.hpp"
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include "testUtilities.hpp"

TEST_CASE("PlatformUtilities.name")
{
    CHECK(strcmp(slog::program_short_name(), "slogTest") == 0);
}

TEST_CASE("PlatformUtilities.zulu")
{
    struct tm datetime{};
    time_t t = 3600 + 139;
    slog::zulu_time_r(&datetime, &t);
    CHECK(datetime.tm_hour == 1);
    CHECK(datetime.tm_min == 2);
    CHECK(datetime.tm_sec == 19);
}

TEST_CASE("PlatformUtilities.make_directory")
{
    CHECK(slog::make_directory("foo/bar/baz"));
    FILE* f = fopen("foo/bar/baz/buzz", "w");
    CHECK(f != nullptr);
    std::remove("foo/bar/baz/buzz");
    slog::rmdir("foo/bar/baz");
    slog::rmdir("foo/bar");
    slog::rmdir("foo");
}

TEST_CASE("PlatformUtilities.make_directory.strange")
{
    CHECK(slog::make_directory("./"));
    CHECK(slog::make_directory("/usr"));
    CHECK_FALSE(slog::make_directory("/bad_permissions_not_allowed"));
    CHECK(slog::make_directory("/tmp/slog"));
    slog::rmdir("/tmp/slog");
    CHECK(slog::make_directory("/tmp///slog"));
    slog::rmdir("/tmp/slog");
    CHECK(slog::make_directory("/tmp/slog/"));
    slog::rmdir("/tmp/slog");
    CHECK(slog::make_directory("/tmp/slog///"));
    slog::rmdir("/tmp/slog");
}