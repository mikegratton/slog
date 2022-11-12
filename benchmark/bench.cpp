//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//
// Modified for slog by M.B. Gratton

#include "slog.hpp"
#include "LogSetup.hpp"
#include "FileSink.hpp"
#include "JournaldSink.hpp"

#include <atomic>
#include <cstdlib> // EXIT_FAILURE
#include <memory>
#include <string>
#include <thread>
#include <iostream>

void bench_mt(int howmany, slog::LogConfig& config, size_t thread_count);


static const size_t file_size = 30 * 1024 * 1024;
static const size_t rotating_files = 5;
static const int max_threads = 1000;

void bench_threaded_logging(size_t threads, int iters) {
    std::cout << "**************************************************************\n";
    std::cout << "Threads: " << threads << ", messages: " << iters << "\n";
    std::cout << "**************************************************************\n";
    
    {
    std::cout << "File sink\n";
    slog::LogConfig config;
    auto sink = std::make_shared<slog::FileSink>();
    sink->set_file("./", "bench");
    sink->set_echo(false);    
    config.set_sink(sink);
    config.set_default_threshold(slog::INFO);    
    bench_mt(iters, config, threads);
    }

    {
    std::cout << "Journal sink\n";    
    slog::LogConfig config;
    auto sink = std::make_shared<slog::JournaldSink>();
    sink->set_echo(false);
    config.set_sink(sink);    
    config.set_default_threshold(slog::INFO);
    bench_mt(iters, config, threads);
    }
    
    {        
    std::cout << "Null sink\n";    
    slog::LogConfig config;
    config.set_sink(std::make_shared<slog::NullSink>());
    config.set_default_threshold(slog::INFO);
    bench_mt(iters, config, threads);
    }
}

int main(int argc, char* argv[]) {
    int iters = 250000;
    size_t threads = 4;

    if (argc > 1) {
        iters = std::stoi(argv[1]);
    }
    if (argc > 2) {
        threads = std::stoul(argv[2]);
    }

    if (threads > max_threads) {
        std::cerr << "Noooo.\n";
        return -1;
    }

    bench_threaded_logging(1, iters);
    bench_threaded_logging(threads, iters);

    return 0;
}

void bench_mt(int howmany, slog::LogConfig& config, size_t thread_count) {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;

    slog::start_logger(config);
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    auto start = high_resolution_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([&]() {
            for (int j = 0; j < howmany / static_cast<int>(thread_count); j++) {
                Slog(INFO) << "Hello logger: msg number " << j << " from " << std::this_thread::get_id();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    };
    auto delta_pre = high_resolution_clock::now() - start;
    auto delta_pre_d = duration_cast<duration<double>>(delta_pre).count();
    slog::stop_logger(); // Force backend to drain queue
    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    std::cout << "Elapsed: " << delta_d << " secs " << int(howmany / delta_d) 
    << " msg/sec, pre: " << delta_pre_d << " sec, " << int(howmany / delta_pre_d) 
    << "\n";
}
