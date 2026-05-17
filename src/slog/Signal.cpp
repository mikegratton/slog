#include "Signal.hpp"
#include <thread>
#include "PlatformUtilities.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <csignal>

namespace slog
{

/// All signals handled by Slog
std::array<int, 3> const HANDLED_SIGNALS = {SIGINT, SIGTERM, SIGABRT};

/// Track the signal being handled
std::atomic_int static g_signal_state{SLOG_STOPPED};

/// Count the workers that have reported being finished
std::atomic_int static g_worker_stopped{0};

/// Count the workers that have reported starting
std::atomic_int static g_worker_started{0};

int get_signal_state() { return g_signal_state.load(); }

void set_signal_state(int signal_id) { g_signal_state.store(signal_id); }

void notify_worker_stopping() { g_worker_stopped++; }

void notify_worker_starting() { g_worker_started++; }

void reset_worker_counts()
{
    g_worker_started.store(0);
    g_worker_stopped.store(0);
}

void block_until_all_workers_done()
{
    while (g_worker_stopped < g_worker_started) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto it = std::find(HANDLED_SIGNALS.begin(), HANDLED_SIGNALS.end(), g_signal_state.load());
    if (it != HANDLED_SIGNALS.end()) {
        slog::forward_signal(g_signal_state);
    }
}

} // namespace slog

extern "C" void slog_handle_signal(int signal)
{
    slog::set_signal_state(signal);
    slog::block_until_all_workers_done();        
}

extern "C" void slog_handle_exit() 
{ 
    slog_handle_signal(slog::SLOG_STOPPED);
}