#include "Signal.hpp"
#include "LoggerSingleton.hpp"
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

/// Count the channels that have reported being flished
std::atomic_int static g_reported_in{0};

int get_signal_state() { return g_signal_state.load(); }

void set_signal_state(int signal_id)
{
    g_signal_state.store(signal_id);
    if (signal_id == SLOG_ACTIVE) {
        g_reported_in.store(0);
    }
}

void notify_channel_done() { g_reported_in++; }

void block_until_all_channels_done()
{
    while (g_reported_in < static_cast<int>(detail::Logger::channel_count())) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto it = std::find(HANDLED_SIGNALS.begin(), HANDLED_SIGNALS.end(), g_signal_state.load());
    if (it != HANDLED_SIGNALS.end()) {
        slog::forward_signal(g_signal_state);
        std::this_thread::sleep_for(std::chrono::hours(1000000));
    }
}

} // namespace slog

extern "C" void slog_handle_signal(int signal)
{
    slog::set_signal_state(signal);
    slog::block_until_all_channels_done();
}

extern "C" void slog_handle_exit() { slog::detail::Logger::stop_all_channels(); }