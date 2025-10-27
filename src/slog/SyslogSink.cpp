#include "SyslogSink.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
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

SyslogSink::SyslogSink(char const* destination, bool use_tcp)
    : mformat(default_format),
      mecho(true),
      msyslog_facility(kSyslogUserFacility),
      muse_rfc3164(false),
      muse_tcp(use_tcp),
      msock_fd(-1),
      munix_socket(nullptr)
{
    int status;
    strncpy(mdestination, destination, sizeof(mdestination) - 1);
    set_application_name(::slog::program_short_name());
    gethostname(mhostname, sizeof(mhostname) - 1);

    // Set up the FILE memory stream
    mbufferStream = open_memstream(&mbuffer, &mbuffer_size);
    connect();
}

SyslogSink::~SyslogSink()
{
    if (is_connected()) {
        disconnect();
    }
    fclose(mbufferStream);
    free(mbuffer);
}

bool SyslogSink::connect()
{
    if (is_connected()) {
        disconnect();
    }

    int protocol = (muse_tcp ? SOCK_STREAM : SOCK_DGRAM);
    int status;
    // Check which kind of socket we're using
    if (mdestination[0] == '/') {
        muse_tcp = false;
        // Get the socket
        msock_fd = socket(AF_UNIX, protocol, 0);

        // Bind to a temp address
        if (protocol == SOCK_DGRAM) {
            sockaddr_un address{0};
            make_unix_socket();
            strncpy(address.sun_path, munix_socket, sizeof(address.sun_path) - 1);
            address.sun_family = AF_UNIX;
            auto* caddress = reinterpret_cast<sockaddr*>(&address);
            auto address_size = sizeof(sockaddr_un);
            status = bind(msock_fd, caddress, address_size);
            if (status < 0) {
                slog_error("Faild to bind to unix socket\n");
                disconnect();
                return false;
            }
        }

        // Connect to server's socket
        sockaddr_un address{0};
        strncpy(address.sun_path, mdestination, sizeof(address.sun_path) - 1);
        address.sun_family = AF_UNIX;
        auto* caddress = reinterpret_cast<sockaddr*>(&address);
        auto address_size = sizeof(sockaddr_un);
        status = ::connect(msock_fd, caddress, address_size);
        if (status < 0) {
            slog_error("Failed to connect to %s\n", mdestination);
            disconnect();
            return false;
        }
    } else {
        // Get the socket
        msock_fd = socket(AF_INET, protocol, 0);
        sockaddr_in address{0};
        addrinfo hints{0};
        addrinfo* info;
        char* port;
        for (port = mdestination; *port != '\0'; port++) {
            if (*port == ':') {
                *port = 0;
                port = port + 1;
                break;
            }
        }
        hints.ai_family = AF_UNSPEC;  // don't care IPv4 or IPv6
        hints.ai_socktype = protocol; // socket type
        hints.ai_flags = AI_PASSIVE;  // fill in my IP for me // should be 0?
        if ((status = getaddrinfo(mdestination, port, &hints, &info)) != 0) {
            slog_error("getaddrinfo error: %s\n", gai_strerror(status));
            disconnect();
            return false;
        }
        if (info) {
            memcpy(&address, info->ai_addr, info->ai_addrlen);
            freeaddrinfo(info);
        } else {
            slog_error("Could not resolve hostname %s\n", mdestination);
            freeaddrinfo(info);
            disconnect();
            return false;
        }
        auto* caddress = reinterpret_cast<sockaddr*>(&address);
        auto address_size = sizeof(sockaddr_in);
        status = ::connect(msock_fd, caddress, address_size);
        if (status == -1) {
            slog_error("Failed to connect to %s\n", mdestination);
            disconnect();
            return false;
        }
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

    munix_socket = new char[socket_size];

    auto app_size = strnlen(mapplication_name, sizeof(mapplication_name));
    if (dir_size + app_size + stem_size + tmp_size + 1 >= socket_size) {
        slog_error("Unix socket path too long for syslog\n");
        snprintf(munix_socket, socket_size - 1, "/tmp/slog%s%s", tmp_text, stem);
        return munix_socket;
    }

    snprintf(munix_socket, socket_size - 1, "%s/%s%s%s", directory, mapplication_name, tmp_text, stem);
    return munix_socket;
}

bool SyslogSink::is_connected() const { return msock_fd != -1; }

void SyslogSink::disconnect()
{
    if (is_connected()) {
        shutdown(msock_fd, SHUT_WR);
        close(msock_fd);
        if (munix_socket) {
            std::remove(munix_socket);
            delete[] munix_socket;
            munix_socket = nullptr;
        }
    }
    msock_fd = -1;
}

void SyslogSink::finalize() { disconnect(); }

int SyslogSink::syslog_priority(int slog_severity) const
{
    int syslogSeverity = slog_severity / 100;
    if (syslogSeverity < 0) {
        syslogSeverity = 0;
    } else if (syslogSeverity > 7) {
        syslogSeverity = 7;
    }
    int priority = msyslog_facility * 8 + syslogSeverity;
    return priority;
}

void SyslogSink::record(LogRecord const& node)
{
    // TODO loop to connect?
    if (!is_connected()) {
        connect();
    }

    rewind(mbufferStream);
    int bytesWritten = 0;

    // write the header
    int headerOffset = format_header(node);
    bytesWritten += headerOffset;

    // Format the message into the buffer
    bytesWritten += mformat(mbufferStream, node);
    fflush(mbufferStream);

    if (mecho) {
        fwrite(mbuffer + headerOffset, sizeof(char), bytesWritten - headerOffset, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }

    // Truncate the message if we're using UDP/IP
    if (bytesWritten > kMaxDatagramSize && muse_tcp == false && munix_socket == nullptr) {
        bytesWritten = kMaxDatagramSize;
    }

    // Send the buffer
    if (is_connected()) {
        int status;
        if (muse_tcp) {
            // Send the octet count to delimit the message
            char sizeInfo[16];
            int sizeInfoSize = snprintf(sizeInfo, sizeof(sizeInfo), "%d ", bytesWritten);
            send(msock_fd, sizeInfo, sizeInfoSize, 0);
        }
        status = send(msock_fd, mbuffer, bytesWritten, 0);
        if (status == -1) {
            slog_error("Failed to send with code %d\n", errno);
            disconnect();
        }
    }
}

int SyslogSink::format_header(LogRecord const& node)
{
    int headerOffset = 0;
    int priority = syslog_priority(node.meta.severity);
    if (munix_socket) {
        headerOffset = fprintf(mbufferStream, "<%d> %s: ", priority, mapplication_name);
    } else if (muse_rfc3164) {
        format_time(mtimestamp, node.meta.time, 3, FULL_T);
        headerOffset = fprintf(mbufferStream, "<%d> %s %s %s: ", priority, mtimestamp, mhostname, mapplication_name);
    } else {
        format_time(mtimestamp, node.meta.time, 3, FULL_T);
        headerOffset = fprintf(mbufferStream, "<%d>1 %s %s %s - %s - ", priority, mtimestamp, mhostname,
                               mapplication_name, (node.meta.tag[0] ? node.meta.tag : "-"));
    }
    return headerOffset;
}

void SyslogSink::set_application_name(char const* application_name)
{
    strncpy(mapplication_name, application_name, sizeof(mapplication_name) - 1);
}

void SyslogSink::set_facility(int facility)
{
    if (facility < 0) {
        msyslog_facility = 0;
    } else if (facility > 23) {
        msyslog_facility = 23;
    } else {
        msyslog_facility = facility;
    }
}

void SyslogSink::set_rfc3164_protocol(bool doit) { muse_rfc3164 = doit; }

} // namespace slog
