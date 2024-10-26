#include "Business.hpp"

#include "slog/slog.hpp"

void do_business()
{
    double x = 2.5;
    Slog(INFO) << "Hello on info, this is x=" << x;
}
