#pragma once
#include <array>

namespace slog
{

/// All signals handled by slog
std::array<int, 3> extern const HANDLED_SIGNALS;

/// Get the current signal being handled, or the special signal_state codes
/// below if nothing is being handled
int get_signal_state();

/// Start handling the given signal. If this isn't SLOG_ACTIVE, the log channels
/// will flush, and the logging threads will joint.
void set_signal_state(int signal_id);

/// Channels call this when they join their log threads
void notify_channel_done();

/// Block until all channels have called notify_channel_done(). If get_signal_state()
/// has one of HANDLED_SIGNALS, then the we will re-raise the signal with the default
/// handler in place.
void block_until_all_channels_done();

/// Extra "signal" codes that are special to slog
enum signal_state : int {
    SLOG_ACTIVE = 0,  // Logging is active
    SLOG_STOPPED = ~0 // Logging is inactive, but there's no signal being handled
};

} // namespace slog

/// Exit handler. Stop all channels and record remaining messages in the queue.
extern "C" void slog_handle_exit();