#include "doctest.h"
#include "slog.hpp"
#include "LogSetup.hpp"
#include <ostream>


TEST_CASE("Basic Stream") {
    slog::start_logger(INFO);
    Log(DBUG) <<  "Can't see this";
    Log(INFO) << "Can see this";
    Log(ERRR) << "This is bad";
    Log(NOTE) << "This is note";
    Log(WARN) << "This is warn";
    slog::stop_logger();
}
