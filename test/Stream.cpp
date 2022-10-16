#include "doctest.h"
#include "slog.hpp"
#include "LogSetup.hpp"
#include <ostream>


TEST_CASE("Basic Stream") {
    slog::start_logger(slog::INFO);
    Slog(DBUG) <<  "Can't see this";
    Slog(INFO) << "Can see this";
    Slog(ERRR) << "This is bad";
    Slog(NOTE) << "This is note";
    Slog(WARN) << "This is warn";
    slog::stop_logger();
    Slog(DBUG) <<  "Post stop log";
}
