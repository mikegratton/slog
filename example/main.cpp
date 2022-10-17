#include "LogSetup.hpp"
#include "Business.hpp"

int main(int, char**)
{
    slog::start_logger(slog::INFO);
    do_business();
    return 0;
}
