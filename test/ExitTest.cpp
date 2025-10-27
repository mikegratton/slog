#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include "SlowSink.hpp"
#include <chrono>
#include <cstring>
#include <memory>
#include <thread>
#include <unistd.h>

std::shared_ptr<SlowSink> g_sink;

TEST_CASE("ExitTest")
{
    int id = fork();
    if (id == 0) {
        printf("Hello from child\n");
        g_sink = std::make_shared<SlowSink>();        
        printf("Made sink\n");
        slog::LogConfig config;
        config.set_sink(g_sink);
        slog::start_logger(config);
        printf("Start logging on child\n");
        Slog(NOTE) << "Test record 0";
        Slog(NOTE) << "Test record 1";
        Slog(NOTE) << "Test record 2";
        Slog(NOTE) << "Test record 3";
        Slog(NOTE) << "Test record 4";
        Slog(NOTE) << "Test record 5";
        g_sink->unlock();
        printf("Sink unlocked\n");
        exit(0);
    } else {
        printf("Hello from parent\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        FILE* f = nullptr;
        for (int i = 0; i < 10; i++) {
            printf("Attempt %d to open %s\n", i,SlowSink::file_name());
            f = fopen(SlowSink::file_name(), "r");
            if (f) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        char buffer[1024];
        fgets(buffer, sizeof(buffer), f);
        CHECK(strncmp(buffer, "Test record\n", 64) == 0);

        fclose(f);
        //std::remove(SlowSink::file_name());
    }
}