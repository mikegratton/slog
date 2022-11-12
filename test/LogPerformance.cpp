#include "doctest.h"
#include "slog.hpp"
#include "LogSetup.hpp"
#include <chrono>
#include <iostream>

using namespace slog;

/*
 * Baseline 660 ns/log 
 * Basic wait improvement 650 ns/log (not significant)
 * Non-atomic 670 ns/log
 * std::map  780 ns/log 
 * std::unordered_map 840 ns/log
 * No thread-local: 630 ns/log (significant, but small)
 */
TEST_CASE("NullPerformance")
{
    LogConfig config;    
    config.set_sink(std::make_shared<NullSink>());
    config.set_default_threshold(slog::DBUG);
    start_logger(config);
    unsigned long JOB_SIZE = 10000000UL;
    auto start_time = std::chrono::system_clock::now();
    for (unsigned long i = 0; i < JOB_SIZE; i++) {
        Flog(NOTE, "Hello");
    }
    auto stop_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli>  elapsed = stop_time - start_time;
    std::cout << "Null logged " << JOB_SIZE << " records in " << elapsed.count() << " ms \n";
    std::cout << "Per log: " << (elapsed.count()/JOB_SIZE) << " ms/log\n";
    stop_logger();
}

TEST_CASE("RejectPerformance")
{
    LogConfig config;    
    config.set_sink(std::make_shared<NullSink>());
    config.set_default_threshold(slog::ERRR);
    start_logger(config);
    unsigned long JOB_SIZE = 10000000UL;
    auto start_time = std::chrono::system_clock::now();
    for (unsigned long i = 0; i <JOB_SIZE; i++) {
        Flog(NOTE, "Hello");
    }
    auto stop_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli>  elapsed = stop_time - start_time;
    std::cout << "Reject logged " << JOB_SIZE << " records in " << elapsed.count() << " ms \n";
    std::cout << "Per log: " << (elapsed.count()/JOB_SIZE) << " ms/log\n";
    stop_logger();
}


TEST_CASE("RejectTagPerformance")
{
    LogConfig config;    
    config.set_sink(std::make_shared<NullSink>());
    config.set_default_threshold(slog::ERRR);
    config.add_tag("moose", INFO);    
    start_logger(config);
    unsigned long JOB_SIZE = 10000000UL;
    auto start_time = std::chrono::system_clock::now();
    for (unsigned long i = 0; i <JOB_SIZE; i++) {
        Flogt(NOTE, "meep", "Hello");
    }
    auto stop_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli>  elapsed = stop_time - start_time;
    std::cout << "Reject logged " << JOB_SIZE << " records in " << elapsed.count() << " ms \n";
    std::cout << "Per log: " << (elapsed.count()/JOB_SIZE) << " ms/log\n";
    stop_logger();
}
