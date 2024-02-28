#include "SyslogSink.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
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

#include "slog/LogSink.hpp"

#define SLOG_PRINT_SYSLOG_ERROR

namespace slog {

namespace {
void syslog_error(char const* format_, ...)
{
#ifdef SLOG_PRINT_SYSLOG_ERROR
    va_list vlist;
    va_start(vlist, format_);
    vfprintf(stderr, format_, vlist);
    va_end(vlist);
#endif
}
}  // namespace

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
    set_application_name(program_invocation_short_name);
    gethostname(mhostname, sizeof(mhostname) - 1);

    // Set up the FILE memory stream
    mbufferStream = open_memstream(&mbuffer, &mbuffer_size);
    connect();
}

SyslogSink::~SyslogSink()
{
    if (is_connected()) { disconnect(); }
    fclose(mbufferStream);
    free(mbuffer);
    if (munix_socket) {
        unlink(munix_socket);
        delete[] munix_socket;
    }
}

bool SyslogSink::connect()
{
    if (is_connected()) { disconnect(); }

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
                syslog_error("Faild to bind to unix socket\n");
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
            syslog_error("Failed to connecto to %s\n", mdestination);
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
        hints.ai_family = AF_UNSPEC;   // don't care IPv4 or IPv6
        hints.ai_socktype = protocol;  // socket type
        hints.ai_flags = AI_PASSIVE;   // fill in my IP for me
        if ((status = getaddrinfo(mdestination, port, &hints, &info)) != 0) {
            syslog_error("getaddrinfo error: %s\n", gai_strerror(status));
            disconnect();
            return false;
        }
        if (info) {
            memcpy(&address, info->ai_addr, info->ai_addrlen);
            freeaddrinfo(info);
        } else {
            syslog_error("Could not resolve hostname %s\n", mdestination);
            freeaddrinfo(info);
            disconnect();
            return false;
        }
        auto* caddress = reinterpret_cast<sockaddr*>(&address);
        auto address_size = sizeof(sockaddr_in);
        status = ::connect(msock_fd, caddress, address_size);
        if (status == -1) {
            syslog_error("Failed to connect to %s\n", mdestination);
            disconnect();
            return false;
        }
    }
    return true;
}

char* SyslogSink::make_unix_socket()
{
    constexpr int socket_size = 128;
    constexpr int tmp_size = 16;
    constexpr int app_name_size = socket_size - (5 + tmp_size + 8);
    char const* alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
    assert(strlen(alphabet) == 64);

    munix_socket = new char[socket_size];
    auto offset = sprintf(munix_socket, "/tmp/%.*s", app_name_size, program_invocation_short_name);
    FILE* rand = fopen("/dev/urandom", "r");
    char* cursor = munix_socket + offset;
    for (int i = 0; i < tmp_size; i++) {
        char c = fgetc(rand) >> 2;
        assert(c >= 0 && c <= 63);
        c = alphabet[c];
        *cursor++ = c;
    }
    fclose(rand);
    offset += tmp_size;
    sprintf(cursor, ".socket");
    return munix_socket;
}

bool SyslogSink::is_connected() const
{
    return msock_fd != -1;
}

void SyslogSink::disconnect()
{
    if (is_connected()) { shutdown(msock_fd, 0); }
    msock_fd = -1;
}

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
    if (!is_connected()) { connect(); }

    rewind(mbufferStream);
    int bytesWritten = 0;

    // write the header
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
    bytesWritten += headerOffset;

    // Format the message into the buffer
    bytesWritten += mformat(mbufferStream, node);
    fflush(mbufferStream);

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
            syslog_error("Failed to send with code %d\n", errno);
            disconnect();
        }
    }
    if (mecho) {
        fwrite(mbuffer + headerOffset, sizeof(char), bytesWritten - headerOffset, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
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

void SyslogSink::set_rfc3164_protocol(bool doit)
{
    muse_rfc3164 = doit;
}

}  // namespace slog
