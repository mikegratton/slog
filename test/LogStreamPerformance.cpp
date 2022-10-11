#include "doctest.h"
#include "slog.hpp"
#include "LogSetup.hpp"
#include <chrono>
#include <iostream>

using namespace slog;

/*
 * Baseline 470 ns/log !
 */
TEST_CASE("StreamNullPerformance") {
    LogConfig config;
    config.set_sink(new NullSink);
    config.set_default_threshold(DBUG);
    start_logger(config);
    unsigned long JOB_SIZE = 10000000UL;
    auto start_time = std::chrono::system_clock::now();
    for (unsigned long i = 0; i < JOB_SIZE; i++) {
        Log(NOTE) << "Hello";
    }
    auto stop_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli>  elapsed = stop_time - start_time;
    std::cout << "Null stream logged " << JOB_SIZE << " records in " << elapsed.count() << " ms \n";
    std::cout << "Per log: " << (elapsed.count()/JOB_SIZE) << " ms/log\n";
    stop_logger();
}

TEST_CASE("StreamRejectPerformance") {
    LogConfig config;
    config.set_sink(new NullSink);
    config.set_default_threshold(ERRR);
    start_logger(config);
    unsigned long JOB_SIZE = 10000000UL;
    auto start_time = std::chrono::system_clock::now();
    for (unsigned long i = 0; i <JOB_SIZE; i++) {
        Log(NOTE) << "Hello";
    }
    auto stop_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli>  elapsed = stop_time - start_time;
    std::cout << "Reject logged " << JOB_SIZE << " records in " << elapsed.count() << " ms \n";
    std::cout << "Per log: " << (elapsed.count()/JOB_SIZE) << " ms/log\n";
    stop_logger();
}


TEST_CASE("StreamRejectTagPerformance") {
    LogConfig config;    
    config.set_sink(new NullSink);
    config.set_default_threshold(ERRR);
    config.add_tag("moose", INFO); 
    start_logger(config);
    unsigned long JOB_SIZE = 10000000UL;
    auto start_time = std::chrono::system_clock::now();
    for (unsigned long i = 0; i <JOB_SIZE; i++) {
        Log(NOTE, "meep") <<  "Hello";
    }
    auto stop_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli>  elapsed = stop_time - start_time;
    std::cout << "Reject logged " << JOB_SIZE << " records in " << elapsed.count() << " ms \n";
    std::cout << "Per log: " << (elapsed.count()/JOB_SIZE) << " ms/log\n";
    stop_logger();
}
