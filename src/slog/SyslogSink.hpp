#pragma once

#include "LogSink.hpp"
#include <chrono>

struct sockaddr;

namespace slog {

/**
 * @brief A log sink compatible with syslog. Messages are sent over unix sockets
 * or internet sockets.
 *
 * By default, this logger sends RFC5424-formatted records over UDP. When
 * sending TCP messages, it uses the RFC 6587 "Octet Counting" scheme, where the
 * syslog message is preceded by the byte count to be sent, e.g. "57 <12>
 * 2000-01-01T00:00:00Z mydomain.com my_app Hello World" for RFC3164 formatting.
 *
 * @note This can only send messages up to 64kB if the protocol is UDP/IP.
 * Longer messages will be silently truncated. Messages sent to a unix socket
 * via UDP or over TCP/IP do not have this limitation.
 */
class SyslogSink : public LogSink {
   public:
    static constexpr int kSyslogUserFacility = 1;

    /**
     * @brief Construct a sink sending messages to the given location
     * @param destination "<hostname>:<port>" for internet sockets, or an absolute path for unix sockets
     * @param use_tcp Use TCP (UDP is default).
     */
    SyslogSink(char const* destination = "/dev/log", bool use_tcp = false);
    ~SyslogSink();

    /// Write the record to the socket
    void record(LogRecord const& node) final;

    /// Change the formatting for each record (default is default_format)
    void set_formatter(Formatter format) { mformat = format; }

    /// Set echo to console on/off (default is on)
    void set_echo(bool doit = true) { mecho = doit; }

    /// Change the application name (default is slog::program_short_name())
    void set_application_name(char const* application_name);

    /// Change the syslog facility (default is 1, user facility)
    void set_facility(int facility);

    /// Use RFC3164 format messages (default is RFC5424)
    void set_rfc3164_protocol(bool doit);

    /// Set the maximum connect tries before dropping a message (default is 10)
    void set_max_connection_attempts(int attempts);

    /// Set the time to wait *per connection attempt* (default is 50ms)
    void set_max_connection_wait(std::chrono::milliseconds wait);

    /// Close the socket
    void finalize() override;

   protected:
    /// Compute the syslog priority from the slog severity
    int syslog_priority(int slog_severity) const;

    /// Connect sockets (for UDP this just means setting a send address)
    bool connect();

    /// Disconnect sockets
    void disconnect();

    /// Check if we're connected (for UDP this just means setting an address)
    bool is_connected() const;

    /// Make a valid unique path for the unix socket to bind to
    char* make_unix_socket();

    /// Write the syslog header into the buffer, returning the header size
    int format_header(LogRecord const& node);

    /// Connect to a unix socket 
    bool connect_unix();

    /// Connect to a IP socket
    bool connect_ip();

    std::size_t constexpr static kMaxDatagramSize = 65507;
    

    Formatter mformat;
    bool mecho;
    int msyslog_facility;
    bool muse_rfc3164;
    bool muse_tcp;
    int mmax_connection_attempts;
    std::chrono::milliseconds mconnection_attempt_wait;

    int msock_fd;

    char* mbuffer;
    std::size_t mbuffer_size;
    FILE* mbufferStream;

    char mdestination[1024];
    char mapplication_name[64];
    char mhostname[1024];
    char mtimestamp[32];
    char* munix_socket;
};

}  // namespace slog
