#include "slog/LogSetup.hpp"
#include "slog/slog.hpp"
#include "slowSink.hpp"
#include <chrono>
#include <memory>

#include <csignal>
#include <signal.h>
#include <cstring>
#include <cassert>
#include <iostream>
#include <thread>

std::shared_ptr<SlowSink> g_slowSink;

void test_action_simple(int signal_id)
{
    std::cout << "test_action_simple(): Test action (simple) called with signal " << signal_id << std::endl;
}


void install_handler(int signal) {
    struct sigaction old_handler;
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigfillset(&action.sa_mask);
    action.sa_handler = test_action_simple;
    action.sa_flags = 0;
    int status = sigaction(signal, &action, nullptr);
    assert(status == 0);        
}

struct sigaction g_slogHandler{};

void unlock_handler(int signal) {
    std::cout << "unlock_handler(): Unlocking sink" << std::endl;
    g_slowSink->unlock();
    printf("Re-raising signal %d with handler %p\n", signal, g_slogHandler.sa_sigaction);
    sigaction(signal, &g_slogHandler, nullptr);
    raise(signal);
}

void install_unlock_handler(int signal) {    
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigfillset(&action.sa_mask);
    action.sa_handler = unlock_handler;
    action.sa_flags = 0;
    int status = sigaction(signal, &action, &g_slogHandler);
    printf("SIGINT unlock handler is at %p replacing %p\n", action.sa_sigaction, g_slogHandler.sa_sigaction);
    assert(status == 0);        
}

void fake_worker()
{
    std::cout << "Logging messages\n";
    for (int i = 0; i < 10; i++) { Slog(INFO) << i; }
    std::cout << "Messages in log buffer\n";
    std::cout << "Waiting for signal..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(18000));
}

int main(int, char**) 
{
    g_slowSink = std::make_shared<SlowSink>();
    slog::LogConfig config;
    config.set_sink(g_slowSink);
    config.set_default_threshold(slog::INFO);

    slog::start_logger(config);
    sigaction(SIGTERM, nullptr, &g_slogHandler);
    printf("Current handler for %d is %p\n", SIGTERM, g_slogHandler.sa_sigaction);

    install_unlock_handler(SIGTERM);

    auto work_thread = std::thread(fake_worker);
    raise(SIGTERM);
    work_thread.join();
    std::cout << "The numbers 0-9 should be logged\n";
    
    return 0;
}