#include "doctest.h"
#include "slog/slog.hpp"

TEST_CASE("Flog")
{
    slog::start_logger();
    Flog(DBUG, "Invisible");
    Flog(INFO, "Info text %d", 7);
    Flog(WARN, "Hello %s", "guv");
}