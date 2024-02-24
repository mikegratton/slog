#pragma once
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstdio>

#include "LogSink.hpp"

namespace slog {

/**
 * @brief RFC5424 or RFC3164 syslog log sink. Messages are sent over
 * unix or internet sockets using UDP or TCP.
 *
 * By default, this logger sends RFC5424-formatted records over UDP.
 *
 * @note This can only send messages up to 65535 bytes.
 * Longer messages will be silently truncated.
 */
class SyslogSink : public LogSink {
   public:
    static constexpr int kSyslogUserFacility = 1;

    /**
     * @brief Construct a sink sending messages to the given location
     * @param destination "<hostname>:<port>" for internet sockets, or an absolute path for unix sockets
     * @param protocol Protocol to use (SOCK_DGRAM for UDP or SOCK_STREAM for TCP)
     */
    SyslogSink(char const* destination = "/dev/log", int protocol = SOCK_DGRAM);
    ~SyslogSink();

    /// Write the record to the socket
    virtual void record(LogRecord const& node) override;

    /// Change the formatting for each record
    void set_formatter(Formatter format) { mformat = format; }

    /// Set echo to console on/off
    void set_echo(bool doit = true) { mecho = doit; }

    /// Change the application name
    void set_application_name(char const* application_name);

    /// Change the syslog facility
    void set_facility(int facility);

    void set_rfc3164_protocol(bool doit);

   protected:
    static constexpr std::size_t kMaxDatagramSize = 65535;

    Formatter mformat;
    bool mecho;
    int mfacility;
    bool muse_rfc3164;
    bool mtcp;

    sockaddr* maddress;
    std::size_t maddress_size;
    int msock_fd;

    char* mbuffer;
    FILE* mbufferStream;

    char mapplication_name[128];
    char mhostname[256];
    char mtimestamp[32];
};

}  // namespace slog
