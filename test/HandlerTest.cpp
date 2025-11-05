#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include "SlowSink.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <signal.h>

std::shared_ptr<SlowSink> g_sink;

void unlock_sink()
{
    g_sink->unlock();
}

TEST_CASE("ExitTest")
{
    int id = fork();
    if (id == 0) {
        g_sink = std::make_shared<SlowSink>();        
        slog::LogConfig config(slog::DBUG, g_sink);        
        slog::start_logger(config);
        std::atexit(unlock_sink);        
        Slog(NOTE) << "Test record";        
        exit(0);
    } else {
        FILE* f = nullptr;
        for (int i = 0; i < 10; i++) {
            f = fopen(SlowSink::file_name(), "r");
            if (f) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        REQUIRE(f);
        char buffer[1024];
        fgets(buffer, sizeof(buffer), f);
        CHECK(strncmp(buffer, "Test record\n", 64) == 0);

        fclose(f);
        std::remove(SlowSink::file_name());
    }
}


TEST_CASE("FatalTest")
{
    int id = fork();
    if (id == 0) {
        g_sink = std::make_shared<SlowSink>();
        g_sink->unlock();    
        slog::LogConfig config(slog::DBUG, g_sink);        
        slog::start_logger(config);              
        Slog(FATL) << "Test record";
        Slog(NOTE) << "Not logged";
        exit(0);
    } else {           
        FILE* f = nullptr;
        for (int i = 0; i < 10; i++) {            
            f = fopen(SlowSink::file_name(), "r");
            if (f) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        REQUIRE(f);
        char buffer[1024];
        fgets(buffer, sizeof(buffer), f);
        CHECK(strncmp(buffer, "Test record\n", 64) == 0);
        
        fclose(f);
        std::remove(SlowSink::file_name());
    }
}

TEST_CASE("AbortTest")
{
    int id = fork();
    if (id == 0) {
        g_sink = std::make_shared<SlowSink>();         
        slog::LogConfig config(slog::DBUG, g_sink);        
        slog::start_logger(config);        
        Slog(NOTE) << "Test record";
        g_sink->unlock();
        abort();  
    } else {
        FILE* f = nullptr;
        for (int i = 0; i < 10; i++) {            
            f = fopen(SlowSink::file_name(), "r");
            if (f) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        REQUIRE(f);
        char buffer[1024];
        fgets(buffer, sizeof(buffer), f);
        CHECK(strncmp(buffer, "Test record\n", 64) == 0);

        fclose(f);
        std::remove(SlowSink::file_name());
    }
}


TEST_CASE("InterruptTest")
{
    int id = fork();
    if (id == 0) {
        g_sink = std::make_shared<SlowSink>();         
        slog::LogConfig config(slog::DBUG, g_sink);        
        slog::start_logger(config);
        Slog(NOTE) << "Test record";
        g_sink->unlock();
        printf("Finished\n");
        raise(SIGINT);
    } else {
        FILE* f = nullptr;
        for (int i = 0; i < 10; i++) {
            f = fopen(SlowSink::file_name(), "r");
            if (f) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        REQUIRE(f);
        char buffer[1024];
        fgets(buffer, sizeof(buffer), f);
        CHECK(strncmp(buffer, "Test record\n", 64) == 0);

        fclose(f);        
        std::remove(SlowSink::file_name());
    }
}

TEST_CASE("StartStop")
{
    auto sink = std::make_shared<SlowSink>();
    sink->unlock();
    slog::LogConfig config(slog::DBUG, sink);
    auto* probe = std::signal(SIGINT, nullptr);
    CHECK(reinterpret_cast<void*>(probe) == nullptr);
    slog::start_logger(config);
    slog::stop_logger();
    probe = std::signal(SIGINT, nullptr);
    CHECK(reinterpret_cast<void*>(probe) == nullptr);
    slog::start_logger(config);
    Slog(NOTE) << "Test record";
    slog::stop_logger();
    FILE* f = fopen(SlowSink::file_name(), "r");    
    REQUIRE(f);
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    CHECK(strncmp(buffer, "Test record\n", 64) == 0);
    fclose(f);        
    std::remove(SlowSink::file_name());
}