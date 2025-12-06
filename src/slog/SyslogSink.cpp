#include "SyslogSink.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "LogSink.hpp"
#include "PlatformUtilities.hpp"
#include "SlogError.hpp"

namespace slog
{

SyslogSink::SyslogSink(char const* host, bool use_tcp)
    : format(default_format),
      echo(true),
      syslog_facility(kSyslogUserFacility),
      use_rfc3164(false),
      use_tcp(use_tcp),
      max_connection_attempts(10),
      connection_attempt_wait(std::chrono::milliseconds(50)),
      sock_fd(-1),
      unix_socket(nullptr)
{    
    strncpy(destination, host, sizeof(destination) - 1);
    set_application_name(::slog::program_short_name());
    gethostname(hostname, sizeof(hostname) - 1);

    // Set up the FILE memory stream
    buffer_stream = open_memstream(&buffer, &buffer_size);
    connect();
}

SyslogSink::~SyslogSink()
{
    if (is_connected()) {
        disconnect();
    }
    fclose(buffer_stream);
    free(buffer);
}

bool SyslogSink::connect()
{
    if (is_connected()) {
        disconnect();
    }

    bool connected = false;
    for (int try_count = 0; try_count < max_connection_attempts && !connected;
         ++try_count, std::this_thread::sleep_for(std::chrono::milliseconds(connection_attempt_wait))) {
        if (destination[0] == '/') {
            connected = connect_unix();
        } else {
            connected = connect_ip();
        }
    }
    if (!connected) {
        disconnect();
    }
    return connected;
}

bool SyslogSink::connect_ip()
{
    int protocol = (use_tcp ? SOCK_STREAM : SOCK_DGRAM);
    int status;

    // Get the socket
    sock_fd = socket(AF_INET, protocol, 0);
    if (sock_fd <= 0) {
        slog_error("Could not create IP socket -- %s\n", strerror(errno));
        return false;
    }

    // Fill out the address
    sockaddr_in address{};
    addrinfo hints{};
    addrinfo* info = nullptr;
    char* port;
    for (port = destination; *port != '\0'; port++) {
        if (*port == ':') {
            *port = 0;
            port = port + 1;
            break;
        }
    }
    hints.ai_family = AF_UNSPEC;  // don't care IPv4 or IPv6
    hints.ai_socktype = protocol; // socket type
    hints.ai_flags = AI_PASSIVE;  // fill in my IP for me
    if ((status = getaddrinfo(destination, port, &hints, &info)) != 0) {
        slog_error("getaddrinfo error: %s\n", gai_strerror(status));
        close(sock_fd);
        return false;
    }
    if (info) {
        memcpy(&address, info->ai_addr, info->ai_addrlen);
        freeaddrinfo(info);
    } else {
        slog_error("Could not resolve hostname %s -- %s\n", destination, strerror(errno));
        freeaddrinfo(info);
        close(sock_fd);
        return false;
    }

    // Connect to the socket
    auto* caddress = reinterpret_cast<sockaddr*>(&address);
    auto address_size = sizeof(sockaddr_in);
    status = ::connect(sock_fd, caddress, address_size);
    if (status == -1) {
        slog_error("Failed to connect to %s -- %s\n", destination, strerror(errno));
        close(sock_fd);
        return false;
    }

    return true;
}

bool SyslogSink::connect_unix()
{
    int status;
    // Only udp on unix sockets please
    use_tcp = false;
    // Get the socket
    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd <= 0) {
        slog_error("Failed to create socket -- %s\n", strerror(errno));
        return false;
    }

    // Bind to a temp address
    sockaddr_un address{};
    make_unix_socket();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(address.sun_path, unix_socket, sizeof(address.sun_path) - 1);
#pragma GCC diagnostic pop
    address.sun_family = AF_UNIX;
    auto* caddress = reinterpret_cast<sockaddr*>(&address);
    auto address_size = sizeof(sockaddr_un);
    status = bind(sock_fd, caddress, address_size);
    if (status < 0) {
        slog_error("Failed to bind to unix socket -- %s\n", strerror(errno));
        unlink(unix_socket);
        close(sock_fd);
        return false;
    }

    // Connect to server's socket
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(address.sun_path, destination, sizeof(address.sun_path) - 1);
#pragma GCC diagnostic pop
    address.sun_family = AF_UNIX;
    caddress = reinterpret_cast<sockaddr*>(&address);
    address_size = sizeof(sockaddr_un);
    status = ::connect(sock_fd, caddress, address_size);
    if (status < 0) {
        slog_error("Failed to connect to %s -- %s\n", destination, strerror(errno));
        close(sock_fd);
        unlink(unix_socket);
        return false;
    }
    return true;
}

char* SyslogSink::make_unix_socket()
{
    int constexpr socket_size = 108;
    int constexpr tmp_size = 6;
    char tmp_text[tmp_size + 1];
    int constexpr stem_size = 5;
    char const* stem = ".sock";

    char const* directory = getenv("XDG_RUNTIME_DIR");
    if (nullptr == directory) {
        directory = "/tmp";
    }
    auto dir_size = strnlen(directory, socket_size);

    std::default_random_engine generator(std::random_device{}());
    std::uniform_int_distribution<char> charDist('0', 'z');
    for (int i = 0; i < tmp_size; i++) {
        tmp_text[i] = charDist(generator);
    }
    tmp_text[tmp_size] = '\0';

    unix_socket = new char[socket_size];

    auto app_size = strnlen(application_name, sizeof(application_name));
    if (dir_size + app_size + stem_size + tmp_size + 1 >= socket_size) {
        slog_error("Unix socket path too long for syslog\n");
        snprintf(unix_socket, socket_size - 1, "/tmp/slog%s%s", tmp_text, stem);
        return unix_socket;
    }

    snprintf(unix_socket, socket_size - 1, "%s/%s%s%s", directory, application_name, tmp_text, stem);
    return unix_socket;
}

bool SyslogSink::is_connected() const { return sock_fd != -1; }

void SyslogSink::disconnect()
{
    if (is_connected()) {
        shutdown(sock_fd, SHUT_WR);
        close(sock_fd);
        if (unix_socket) {
            std::remove(unix_socket);
            delete[] unix_socket;
            unix_socket = nullptr;
        }
    }
    sock_fd = -1;
}

void SyslogSink::finalize() { disconnect(); }

int SyslogSink::syslog_priority(int slog_severity) const
{
    int syslog_severity = slog_severity / 100;
    if (syslog_severity < 0) {
        syslog_severity = 0;
    } else if (syslog_severity > 7) {
        syslog_severity = 7;
    }
    int priority = syslog_facility * 8 + syslog_severity;
    return priority;
}

void SyslogSink::record(LogRecord const& node)
{
    if (!is_connected() && !connect()) {
        slog_error("Could not connect to %s, record dropped", hostname);
        return;
    }

    rewind(buffer_stream);
    int bytes_written = 0;

    // write the header
    int header_offset = format_header(node);
    bytes_written += header_offset;

    // Format the message into the buffer
    bytes_written += format(buffer_stream, node);
    fflush(buffer_stream);

    if (echo) {
        fwrite(buffer + header_offset, sizeof(char), bytes_written - header_offset, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }

    // Truncate the message if we're using UDP/IP
    if (bytes_written > MAX_DATAGRAM_SIZE && use_tcp == false && unix_socket == nullptr) {
        bytes_written = MAX_DATAGRAM_SIZE;
    }

    // Send the buffer
    if (is_connected()) {
        int status;
        if (use_tcp) {
            // Send the octet count to delimit the message
            char size_info[16];
            int size_info_size = snprintf(size_info, sizeof(size_info), "%d ", bytes_written);
            send(sock_fd, size_info, size_info_size, 0);
        }
        status = send(sock_fd, buffer, bytes_written, 0);
        if (status == -1) {
            slog_error("Failed to send with code %d\n", errno);
            disconnect();
        }
    }
}

int SyslogSink::format_header(LogRecord const& node)
{
    int headerOffset = 0;
    int priority = syslog_priority(node.meta().severity());
    if (unix_socket) {
        headerOffset = fprintf(buffer_stream, "<%d> %s: ", priority, application_name);
    } else if (use_rfc3164) {
        format_time(timestamp, node.meta().time(), 3, FULL_T);
        headerOffset = fprintf(buffer_stream, "<%d> %s %s %s: ", priority, timestamp, hostname, application_name);
    } else {
        format_time(timestamp, node.meta().time(), 3, FULL_T);
        headerOffset = fprintf(buffer_stream, "<%d>1 %s %s %s - %s - ", priority, timestamp, hostname, application_name,
                               (node.meta().tag()[0] ? node.meta().tag() : "-"));
    }
    return headerOffset;
}

void SyslogSink::set_application_name(char const* new_name)
{
    strncpy(application_name, new_name, sizeof(application_name) - 1);
}

void SyslogSink::set_facility(int facility)
{
    if (facility < 0) {
        syslog_facility = 0;
    } else if (facility > 23) {
        syslog_facility = 23;
    } else {
        syslog_facility = facility;
    }
}

void SyslogSink::set_rfc3164_protocol(bool doit) { use_rfc3164 = doit; }

void SyslogSink::set_max_connection_attempts(int attempts) { max_connection_attempts = std::max(1, attempts); }

void SyslogSink::set_max_connection_wait(std::chrono::milliseconds wait)
{
    connection_attempt_wait = std::max(std::chrono::milliseconds(10), wait);
}

} // namespace slog
