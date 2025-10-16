#pragma once
#include <csignal>
#include <vector>

namespace slog
{
namespace detail
{
using SigAction = struct sigaction;
using LegacySignalHandler = void(int);
using SignalHandler = void(int);

/**
 * If the program is ignoring a signal, don't do anything. Else
 * install our handler. Also don't do anything if the handler is already
 * installed.
 *
 * @return True if handler is set for signal. False if signal is ignored
 */
bool install_handler_if_not_ignored(int signal, SignalHandler handler,
                                    sigset_t mask);

/**
 * If we installed a handler for signal, restore the old handler. If
 * we didn't, don't touch the handler.
 */
void reinstall_old_handler(int signal);

/**
 * Forward the signal to the old handler we replaced. If we didn't
 * install a handler of signal_id, do nothing
 */
void forward_signal(int signal_id);

/**
 * Get the old (e.g. os-provided) handler for the given signal
 */
SigAction* old_handler_for_signal(int signal_id);

/**
 * Store the old handler for the signal (presumably because we installed
 * our own handler)
 */
void store_old_signal_handler(int signal_id, SigAction const& handler);

/**
 * Construct a mask to block other handled signals
 */
sigset_t make_signal_mask(std::vector<int> const& handled_signals);

/**
 * Install this handler for all slog-handled signals
 */
void install_handlers(SignalHandler handler);

/**
 * Remove the slog handler and reinstall the old handlers
 */
void reinstall_old_handlers();

} // namespace detail
} // namespace slog