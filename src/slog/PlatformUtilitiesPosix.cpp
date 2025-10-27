#include "PlatformUtilities.hpp"
#include "SlogError.hpp"
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <linux/limits.h>
#include <map>
#include <signal.h>
#include <sys/stat.h>

namespace slog
{

char const* program_short_name() { return program_invocation_short_name; }

void zulu_time_r(struct tm* o_datetime, time_t const* seconds_since_epoch)
{
    gmtime_r(seconds_since_epoch, o_datetime);
}

bool make_directory(char const* directory, int mode)
{
    if (strnlen(directory, 1) == 0) {
        return true;
    }

    char working_path[PATH_MAX + 1];
    realpath(directory, working_path);
    struct stat path_stat {
    };
    if (0 == stat(directory, &path_stat)) {
        return S_ISDIR(path_stat.st_mode);
    }

    for (char* cursor = working_path + 1; *cursor; ++cursor) {
        if (*cursor == '/') {
            *cursor = '\0';
            // Check if this subpath exists
            if (0 == stat(working_path, &path_stat)) {
                // ... and is a directory
                if (!S_ISDIR(path_stat.st_mode)) {
                    return false;
                }
            } else {
                // Okay, try to make this part
                if (0 != mkdir(working_path, mode)) {
                    slog_error("Slog: Could not make directory %s\n", working_path);
                    return false;
                }
            }
            *cursor = '/';
        }
    }
    if (0 != mkdir(working_path, mode)) {
        slog_error("Could not make directory %s\n", working_path);
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// Signal handling & handler storage
namespace
{

/// All signals handled by Slog
std::array<int, 4> static const HANDLED_SIGNALS = {SIGINT, SIGTERM, SIGHUP, SIGABRT};

using SigAction = struct sigaction;

/**
 * Singleton for storing old signal handlers
 */
class SignalStore
{
  public:
    static SignalStore& instance()
    {
        static SignalStore s_instance;
        return s_instance;
    }

    static SigAction* get_old_handler(int signal_id)
    {
        auto it = instance().old_handler.find(signal_id);
        if (it == instance().old_handler.end()) {
            return nullptr;
        }
        return &it->second;
    }

    static void store_handler(int signal_id, SigAction const& handler) { instance().old_handler[signal_id] = handler; }

  private:
    SignalStore() = default;
    std::map<int, SigAction> old_handler;
};

template <class T> sigset_t make_signal_mask(T const& handled_signals)
{
    sigset_t mask;
    sigemptyset(&mask);
    for (int signal : handled_signals) {
        sigaddset(&mask, signal);
    }
    return mask;
}

void restore_old_signal_handler(int signal_id)
{
    auto* old_handler = SignalStore::get_old_handler(signal_id);
    if (old_handler) {
        if (old_handler->sa_handler == SIG_DFL) {
            printf("Slog: restoring default handler for %d\n", signal_id);
            signal(signal_id, SIG_DFL);
        } else {
            printf("Slog: restoring custom handler for %d\n", signal_id);
            sigaction(signal_id, old_handler, nullptr);
        }
    }
}

bool install_signal_handler_if_not_ignored(int signal_id, signal_handler handler)
{
    SigAction action{};
    action.sa_mask = make_signal_mask(HANDLED_SIGNALS);
    action.sa_handler = handler;

    SigAction current_handler{};
    int status = sigaction(signal_id, nullptr, &current_handler);
    if (status != 0) {
        slog_error("Failed to retrieve handler for signal %d\n", signal_id);
        return false;
    }
    if (current_handler.sa_handler == SIG_IGN || current_handler.sa_handler == handler) {
        // Nothing to do. Either the signal is ignored or our handler is already in place
        return true;
    }
    SignalStore::store_handler(signal_id, current_handler);
    status = sigaction(signal_id, &action, nullptr);
    if (status != 0) {
        slog_error("Slog: Failed to install handler for signal %d\n", signal_id);
        restore_old_signal_handler(signal_id);
        return false;
    }
    return true;
}

} // namespace

void forward_signal(int signal_id)
{
    static std::atomic<bool> s_handleSignal{true};
    if (s_handleSignal) {
        s_handleSignal = false;
        restore_old_signal_handler(signal_id);
        raise(signal_id);
        printf("forward_signal ends\n");
    }
}

void install_signal_handlers(signal_handler handler)
{
    for (int signal_id : HANDLED_SIGNALS) {
        install_signal_handler_if_not_ignored(signal_id, handler);
    }
}

void restore_old_signal_handlers()
{
    for (int signal : HANDLED_SIGNALS) {
        restore_old_signal_handler(signal);
    }
}

} // namespace slog