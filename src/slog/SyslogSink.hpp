#pragma once

#include "LogSink.hpp"

struct sockaddr;

namespace slog {

/**
 * @brief A log sink compatible with syslog. Messages are sent over unix sockets or internet sockets.
 *
 * By default, this logger sends RFC5424-formatted records over UDP. When sending TCP messages, it uses
 * the RFC 6587 "Octet Counting" scheme, where the syslog message is preceded by the byte count to be
 * sent, e.g. "57 <12> 2000-01-01T00:00:00Z mydomain.com my_app Hello World" for RFC3164 formatting.
 *
 * @note This can only send messages up to 64kB if the protocol is UDP/IP. Longer messages will be
 * silently truncated. Messages sent to a unix socket or over TCP/IP do not have this limitation.
 */
class SyslogSink : public LogSink {
   public:
    static constexpr int kSyslogUserFacility = 1;

    /**
     * @brief Construct a sink sending messages to the given location
     * @param destination "<hostname>:<port>" for internet sockets, or an absolute path for unix sockets
     * @param use_tcp Use TCP instead of UDP.
     */
    SyslogSink(char const* destination = "/dev/log", bool use_tcp = false);
    ~SyslogSink();

    /// Write the record to the socket
    void record(LogRecord const& node) final;

    /// Change the formatting for each record (default is default_format)
    void set_formatter(Formatter format) { mformat = format; }

    /// Set echo to console on/off (default is on)
    void set_echo(bool doit = true) { mecho = doit; }

    /// Change the application name (default is program_invocation_short_name)
    void set_application_name(char const* application_name);

    /// Change the syslog facility (default is 1, user facility)
    void set_facility(int facility);

    /// Use RFC3164 format messages (default is RFC5424)
    void set_rfc3164_protocol(bool doit);

   protected:
    int syslog_priority(int slog_severity) const;
    bool connect();
    void disconnect();
    bool is_connected() const;
    char* make_unix_socket();

    static constexpr std::size_t kMaxDatagramSize = 65507;

    Formatter mformat;
    bool mecho;
    int msyslog_facility;
    bool muse_rfc3164;
    bool muse_tcp;

    int msock_fd;

    char* mbuffer;
    std::size_t mbuffer_size;
    FILE* mbufferStream;

    char mdestination[256];
    char mapplication_name[128];
    char mhostname[256];
    char mtimestamp[32];
    char* munix_socket;
};

}  // namespace slog
