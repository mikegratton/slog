#include <thread>

#include "Business.hpp"
#include "slog/ConsoleSink.hpp"
#include "slog/LogSetup.hpp"

int main(int, char**)
{
    slog::start_logger(slog::INFO);
    do_business();

    return 0;
}
