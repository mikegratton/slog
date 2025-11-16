#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include "SlowSink.hpp"
#include "testUtilities.hpp"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>


static int run_test(char const* name)
{
    printf("----------------Child Process Output-----------------------\n");
    // call signalTest/HandlerProgram & get pid    
    std::string command = slog::get_path_to_test() + "/HandlerProgram " + name;
    int pid;
    auto p = slog::popen2(command.c_str(), "r", &pid);
    REQUIRE(p);
    int exit_code = slog::wait_for_child(pid);
    pclose(p);
    printf("-----------------------------------------------------------\n");
    return exit_code;
} 

TEST_CASE("ExitTest")
{    
    int exit_code = run_test("exit");
    CHECK(exit_code == 0);        
    FILE* f = fopen(SlowSink::file_name(), "r");       
    REQUIRE(f);
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    CHECK(strncmp(buffer, "Test record\n", 64) == 0);
    fclose(f);
    std::remove(SlowSink::file_name());
}


TEST_CASE("FatalTest")
{
    int exit_code = run_test("fatal");
    CHECK(exit_code == SIGABRT);
    FILE* f = fopen(SlowSink::file_name(), "r");
    REQUIRE(f);
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    CHECK(strncmp(buffer, "Test record\n", 64) == 0);

    fclose(f);
    std::remove(SlowSink::file_name());
}

TEST_CASE("AbortTest")
{
    int exit_code = run_test("abort");
    CHECK(exit_code == SIGABRT);
    FILE* f = fopen(SlowSink::file_name(), "r");
    REQUIRE(f);
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    CHECK(strncmp(buffer, "Test record\n", 64) == 0);

    fclose(f);
    std::remove(SlowSink::file_name());
}

TEST_CASE("InterruptTest")
{
    int exit_code = run_test("interrupt");
    CHECK(exit_code == SIGINT);
    FILE* f = fopen(SlowSink::file_name(), "r");
    REQUIRE(f);
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    CHECK(strncmp(buffer, "Test record\n", 64) == 0);

    fclose(f);
    std::remove(SlowSink::file_name());
}

TEST_CASE("TermTest")
{
    int exit_code = run_test("term");
    CHECK(exit_code == SIGTERM);
    FILE* f = fopen(SlowSink::file_name(), "r");
    REQUIRE(f);
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    CHECK(strncmp(buffer, "Test record\n", 64) == 0);

    fclose(f);
    std::remove(SlowSink::file_name());
}

TEST_CASE("StartStop")
{
    std::remove(SlowSink::file_name());

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
    probe = std::signal(SIGINT, nullptr);
    CHECK(reinterpret_cast<void*>(probe));
    Slog(NOTE) << "Test record";
    slog::stop_logger();
    
    FILE* f = fopen(SlowSink::file_name(), "r");    
    REQUIRE(f);
    char buffer[1024];
    fgets(buffer, sizeof(buffer), f);
    CHECK_MESSAGE(strncmp(buffer, "Test record\n", 64) == 0, "Read \"", buffer, "\", but expected \"Test record\n\"");
    fclose(f);        
    std::remove(SlowSink::file_name());
}