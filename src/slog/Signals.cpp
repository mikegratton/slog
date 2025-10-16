#include "Signals.hpp"
#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <map>
namespace slog
{
namespace detail
{
namespace
{

// Old signal handlers to be called during shutdown
std::map<int,SigAction> g_oldHandler;

} // namespace

SigAction* old_handler_for_signal(int signal_id)
{
    auto it = g_oldHandler.find(signal_id);
    if (it == g_oldHandler.end()) {
        return nullptr;
    }
    return &it->second;    
}

void store_old_signal_handler(int signal_id, SigAction const& handler)
{
    g_oldHandler[signal_id] = handler;
}

void forward_signal(int signal_id)
{    
    static std::atomic<bool> s_handleSignal{true};
    auto* old_handler = old_handler_for_signal(signal_id);
    if (s_handleSignal && old_handler && old_handler->sa_handler) {               
        s_handleSignal = false;         
        printf("Re-raising signal %d from forward_signal() with handler %p\n", signal_id, old_handler->sa_handler);
        struct sigaction* our_handler;
        if (0 != sigaction(signal_id, old_handler, our_handler)) {
            printf("Could not install old signal handler\n");
        }
        printf("Re-raise signal\n");
        raise(signal_id);
        printf("Handler ends\n");
    } else {
        signal(signal_id, SIG_DFL);
        raise(signal_id);
    }
}

bool install_handler_if_not_ignored(int signal, SignalHandler handler, sigset_t mask)
{
    SigAction action{};
    action.sa_mask = mask;
    action.sa_handler = handler;        

    struct sigaction current_handler;
    int status = sigaction(signal, nullptr, &current_handler);
    assert(status == 0);    
    if (current_handler.sa_handler == SIG_IGN) {
        return false;
    }
    if (current_handler.sa_handler == handler) {
        return true;
    }
    store_old_signal_handler(signal, current_handler);
    status = sigaction(signal, &action, nullptr);
    assert(status == 0);
    printf("Install handler for %d at %p replacing %p\n", signal, action.sa_handler, current_handler.sa_handler);
    return true;
}

static const std::vector<int> kHandledSignals{SIGINT, SIGTERM, SIGHUP, SIGABRT};

void reinstall_old_handlers()
{
    for (int signal : kHandledSignals) {
        reinstall_old_handler(signal);
    }
}

void install_handlers(SignalHandler* handler)
{
    sigset_t signal_mask = make_signal_mask(kHandledSignals);    
    for (int signal_id : kHandledSignals) {
        install_handler_if_not_ignored(signal_id, handler, signal_mask);
    }

}


void reinstall_old_handler(int signal)
{
    auto* old_handler = old_handler_for_signal(signal);
    if (old_handler) {
        sigaction(signal, old_handler, nullptr);
    }    
}

sigset_t make_signal_mask(std::vector<int> const& handled_signals)
{
    sigset_t mask;
    sigemptyset(&mask);
    for (int signal : handled_signals) {
        sigaddset(&mask, signal);
    }
    return mask;
}

} // namespace detail
} // namespace slog