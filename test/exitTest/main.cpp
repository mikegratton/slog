#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include "slowSink.hpp"
#include <chrono>
#include <cstdlib>
#include <memory>

#include <cstring>
#include <cassert>
#include <iostream>
#include <thread>

std::shared_ptr<SlowSink> g_slowSink;

extern "C" void unlocker()
{
    std::cout << "unlocker(): Unlocking sink from exit handler" << std::endl;
    g_slowSink->unlock();
}

void fake_worker()
{
    std::cout << "Logging messages\n";
    for (int i = 0; i < 10; i++) { Slog(INFO) << i; }
    std::cout << "Messages in log buffer\n";
    std::cout << "Waiting for exit()..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(18000));
}

int main(int, char**) 
{

    g_slowSink = std::make_shared<SlowSink>();
    slog::LogConfig config;
    config.set_sink(g_slowSink);
    config.set_default_threshold(slog::INFO);
    slog::start_logger(config);
    std::atexit(unlocker);
    
    auto work_thread = std::thread(fake_worker);
    auto exit_thread = std::thread( []() { 
        std::this_thread::sleep_for(std::chrono::seconds(1));
        exit(EXIT_SUCCESS);
    });
    work_thread.join();
    std::cout << "The numbers 0-9 should be logged\n";
    
    return 0;
}