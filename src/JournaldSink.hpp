#pragma once
#include "LogSink.hpp"
#include <cstdio>

/**
 * Log using the systemd journal. To use this, you must link to libsystemd, 
 * typically via "-lsystemd".
 * 
 * See http://0pointer.de/blog/projects/journalctl.html for info on seeing
 * and handling these log messages
 * 
 */

namespace slog {

class JournaldSink : public LogSink {
public:
    JournaldSink() : mformat(default_format), mecho(false) { }
    ~JournaldSink() = default;

    virtual void record(RecordNode const* node) override;

    /*
     * This only applies to console echo. Structured logging is 
     * used in the journal call
     */
    void set_formatter(Formatter format) { mformat = format; }

    void set_echo(bool doit=true) { mecho = doit; }

protected:

    Formatter mformat;    
    bool mecho;
};

}
