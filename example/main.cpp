#include "LogSetup.hpp"
#include "Business.hpp"
#include "ConsoleSink.hpp"

#include <thread>

int main(int, char**)
{
    slog::start_logger(slog::INFO);
    do_business();
    
    return 0;
}
