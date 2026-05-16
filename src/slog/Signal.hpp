#pragma once
#include <array>

namespace slog
{

/// All signals handled by slog
std::array<int, 3> extern const HANDLED_SIGNALS;

/// Get the current signal being handled, or the special signal_state codes
/// below if nothing is being handled
int get_signal_state();

/// Start handling the given signal. If this isn't SLOG_ACTIVE, the log workers
/// will flush, and the logging threads will joint.
void set_signal_state(int signal_id);

/// Workers call this when they join their log threads
void notify_worker_stopping();

/// Workers call this when they start their log threads
void notify_worker_starting();

/// Reset the started/stopped counts
void reset_worker_counts();

/// Block until all workers have called notify_worker_done(). If get_signal_state()
/// has one of HANDLED_SIGNALS, then the we will re-raise the signal with the default
/// handler in place.
void block_until_all_workers_done();

/// Extra "signal" codes that are special to slog
enum signal_state : int {
    SLOG_ACTIVE = 0,  // Logging is active
    SLOG_STOPPED = ~0 // Logging is inactive, but there's no signal being handled
};

} // namespace slog

/// Exit handler. Stop all channels and record remaining messages in the queue.
extern "C" void slog_handle_exit();