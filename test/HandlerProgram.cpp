#include "SlowSink.hpp"
#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include <cstdio>
#include <map>
#include <string>
#include <functional>
#include <csignal>

std::shared_ptr<SlowSink> g_sink;

static void unlock_sink() { g_sink->unlock(); }

void exit_test()
{
    std::remove(SlowSink::file_name());
    g_sink = std::make_shared<SlowSink>();
    slog::LogConfig config(slog::DBUG, g_sink);
    slog::start_logger(config);
    std::atexit(unlock_sink);
    Slog(NOTE) << "Test record";
    exit(0);
}

void fatal_test()
{
    std::remove(SlowSink::file_name());
    g_sink = std::make_shared<SlowSink>();
    g_sink->unlock();
    slog::LogConfig config(slog::DBUG, g_sink);
    slog::start_logger(config);
    Slog(FATL) << "Test record";
    Slog(NOTE) << "Not logged";
}

void abort_test()
{
    std::remove(SlowSink::file_name());
    g_sink = std::make_shared<SlowSink>();
    slog::LogConfig config(slog::DBUG, g_sink);
    slog::start_logger(config);
    Slog(NOTE) << "Test record";
    g_sink->unlock();
    abort();
}

void interrupt_test()
{
    std::remove(SlowSink::file_name());
    g_sink = std::make_shared<SlowSink>();
    slog::LogConfig config(slog::DBUG, g_sink);
    slog::start_logger(config);
    Slog(NOTE) << "Test record";
    g_sink->unlock();
    raise(SIGINT);    
}

void term_test()
{
    std::remove(SlowSink::file_name());
    g_sink = std::make_shared<SlowSink>();
    slog::LogConfig config(slog::DBUG, g_sink);
    slog::start_logger(config);
    Slog(NOTE) << "Test record";
    g_sink->unlock();
    raise(SIGTERM);    
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: HandlerProgram <test>\n  where <test> can be \"exit\",  \"fatal\", \"abort\", or \"interrupt\"\n");
        return 1;
    }

    std::map<std::string, std::function<void(void)>> test{
        {"exit", exit_test},
        {"fatal", fatal_test},
        {"abort", abort_test},
        {"interrupt", interrupt_test},
        {"term", term_test}
    };
    std::string requested_test(argv[1]);
    auto it = test.find(requested_test);
    if (it == test.end()) {
        fprintf(stderr, "Unknown test \"%s\"\n", argv[1]);
    } else {
        it->second();
    }
    // Indicate we shouldn't reach here
    fprintf(stderr, "Unexpected normal exit from HandlerProgram()\n");
    return 1;
}