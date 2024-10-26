//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//
// Modified for slog by M.B. Gratton

#include <atomic>
#include <cstdlib>  // EXIT_FAILURE
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "slog/FileSink.hpp"
#include "slog/JournaldSink.hpp"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"

void bench_mt(int howmany, slog::LogConfig& config, size_t thread_count);

using namespace std::chrono;

void bench_threaded_logging(size_t threads, int iters)
{
    std::cout << "**************************************************************\n";
    std::cout << "Threads: " << threads << ", messages: " << iters << "\n";
    std::cout << "**************************************************************\n";

    const long messageSize = 256;
    const long poolSize = (messageSize + sizeof(slog::RecordNode)) * iters * threads;
    auto start = high_resolution_clock::now();
    auto pool = std::make_shared<slog::LogRecordPool>(slog::DISCARD, poolSize, messageSize);

    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    std::cout << "Allocated  " << pool->count() << " record nodes in " << delta_d << " secs\n";

    slog::LogConfig config;
    config.set_default_threshold(slog::INFO);
    config.set_pool(pool);
    {
        std::cout << "File sink\n";
        auto sink = std::make_shared<slog::FileSink>();
        sink->set_file("./", "bench");
        sink->set_echo(false);
        config.set_sink(sink);
        bench_mt(iters, config, threads);
        std::cout << "Pool count: " << pool->count() << "\n";
    }

    {
        std::cout << "Journal sink\n";
        auto sink = std::make_shared<slog::JournaldSink>();
        sink->set_echo(false);
        config.set_sink(sink);
        bench_mt(iters, config, threads);
        std::cout << "Pool count: " << pool->count() << "\n";
    }

    {
        std::cout << "Null sink\n";
        config.set_sink(std::make_shared<slog::NullSink>());
        bench_mt(iters, config, threads);
        std::cout << "Pool count: " << pool->count() << "\n";
    }
}

void bench_mt(int howmany, slog::LogConfig& config, size_t thread_count)
{
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;

    slog::start_logger(config);
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    auto start = high_resolution_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
        threads.emplace_back([&]() {
            for (int j = 0; j < howmany; j++) {
                Slog(INFO) << "Hello logger: msg number " << j << " from " << std::this_thread::get_id();
            }
        });
    }

    for (auto& t : threads) { t.join(); };
    auto delta_pre = high_resolution_clock::now() - start;
    auto delta_pre_d = duration_cast<duration<double>>(delta_pre).count();
    slog::stop_logger();  // Force backend to drain queue
    auto delta = high_resolution_clock::now() - start;
    auto delta_d = duration_cast<duration<double>>(delta).count();
    howmany *= thread_count;  // Actual number logged
    std::cout << "Elapsed: " << delta_d << " secs " << int(howmany / delta_d) << " msg/sec, business: " << delta_pre_d
              << " sec, " << int(howmany / delta_pre_d) << " msg/sec\n";
}

int main(int argc, char* argv[])
{
    int iters = 250000;
    size_t threads = 4;

    if (argc > 1) { iters = std::stoi(argv[1]); }
    if (argc > 2) { threads = std::stoul(argv[2]); }

    bench_threaded_logging(1, iters);
    bench_threaded_logging(threads, iters);

    return 0;
}
