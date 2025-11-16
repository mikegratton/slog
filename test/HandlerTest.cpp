#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include "SlowSink.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

std::shared_ptr<SlowSink> g_sink;

static void unlock_sink()
{
    g_sink->unlock();
}

static void wait_for_child(int pid)
{
    int wait_status = 0;
    do {
        waitpid(pid, &wait_status, 0);
        printf("Current status of test child %d is ", pid);
        if (WIFEXITED(wait_status)) {
            printf("exited, status=%d\n", WEXITSTATUS(wait_status));
        } else if (WIFSIGNALED(wait_status)) {
            printf("killed by signal %d\n", WTERMSIG(wait_status));
        } else if (WIFSTOPPED(wait_status)) {
            printf("stopped by signal %d\n", WSTOPSIG(wait_status));
        } else if (WIFCONTINUED(wait_status)) {
            printf("continued\n");
        }
    } while (!WIFEXITED(wait_status) && !WIFSIGNALED(wait_status) && !WCOREDUMP(wait_status));
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
        wait_for_child(id);
        FILE* f = fopen(SlowSink::file_name(), "r");       
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
        wait_for_child(id);
        FILE* f = fopen(SlowSink::file_name(), "r");       
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
        exit(1);
    } else {
        wait_for_child(id);
        FILE* f = fopen(SlowSink::file_name(), "r");       
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
        raise(SIGINT);
        exit(1);
    } else {
        wait_for_child(id);
        FILE* f = fopen(SlowSink::file_name(), "r");       
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