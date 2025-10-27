#include "doctest.h"
#include "slog/ThresholdMap.hpp"

using namespace slog;

TEST_CASE("ThresholdMap")
{
    FlatThresholdMap map;
    map.set_default(INFO);
    map.add_tag("moose", WARN);
    map.add_tag("mongoose", ERRR);
    map.add_tag("mouse", DBUG);
    CHECK(map[""] == INFO);
    CHECK(map["moose"] == WARN);
    CHECK(map["squirrel"] == INFO);
    CHECK(map["mongoose"] == ERRR);
    CHECK(map["mouse"] == DBUG);
    FlatThresholdMap map2 = map;
    CHECK(map2[""] == INFO);
    CHECK(map2["moose"] == WARN);
    CHECK(map2["squirrel"] == INFO);
    CHECK(map2["mongoose"] == ERRR);
    CHECK(map2["mouse"] == DBUG);
}
