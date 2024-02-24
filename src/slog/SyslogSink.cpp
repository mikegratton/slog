#include "SyslogSink.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "slog/LogSink.hpp"

namespace slog {

SyslogSink::SyslogSink(char const* destination, int protocol)
    : mformat(default_format),
      mecho(true),
      mfacility(kSyslogUserFacility),
      muse_rfc3164(false),
      mtcp(protocol == SOCK_STREAM),
      msock_fd(-1)
{
    int status;
    strncpy(mapplication_name, program_invocation_short_name, sizeof(mapplication_name));
    gethostname(mhostname, sizeof(mhostname));

    // Setup the FILE buffer
    mbuffer = new char[kMaxDatagramSize];
    mbufferStream = fmemopen(mbuffer, kMaxDatagramSize, "w");
    setbuf(mbufferStream, nullptr);

    // Check which kind of socket we're using
    if (destination[0] == '/') {
        printf("Unix socket");
        // Get the socket
        msock_fd = socket(AF_UNIX, protocol, 0);
        sockaddr_un* address = reinterpret_cast<sockaddr_un*>(malloc(sizeof(sockaddr_un)));
        maddress_size = sizeof(sockaddr_un);
        strncpy(address->sun_path, destination, sizeof(address->sun_path));
        address->sun_family = AF_UNIX;
        maddress = reinterpret_cast<sockaddr*>(address);
    } else {
        // Get the socket
        msock_fd = socket(AF_INET, protocol, 0);
        sockaddr_in* address = reinterpret_cast<sockaddr_in*>(malloc(sizeof(sockaddr_in)));
        maddress_size = sizeof(sockaddr_in);
        addrinfo hints{0};
        addrinfo* info;
        char destinationT[256];
        strncpy(destinationT, destination, sizeof(destinationT));
        char* port;
        for (port = destinationT; *port != '\0'; port++) {
            if (*port == ':') {
                *port = 0;
                port = port + 1;
                break;
            }
        }
        hints.ai_family = AF_UNSPEC;   // don't care IPv4 or IPv6
        hints.ai_socktype = protocol;  // socket type
        hints.ai_flags = AI_PASSIVE;   // fill in my IP for me
        if ((status = getaddrinfo(destinationT, port, &hints, &info)) != 0) {
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            shutdown(msock_fd, 0);
            msock_fd = -1;
        }
        if (info) {
            memcpy(address, info->ai_addr, info->ai_addrlen);
        } else {
            fprintf(stderr, "Could not resolve hostname %s\n", destination);
            shutdown(msock_fd, 0);
            msock_fd = -1;
        }
        freeaddrinfo(info);
        maddress = reinterpret_cast<sockaddr*>(address);
    }
    status = connect(msock_fd, maddress, maddress_size);
    if (status == -1) {
        fprintf(stderr, "failed to connect to %s\n", destination);
        shutdown(msock_fd, 0);
        msock_fd = -1;
    }
}
SyslogSink::~SyslogSink()
{
    if (msock_fd != -1) { shutdown(msock_fd, 0); }
    fclose(mbufferStream);
    delete[] mbuffer;
    free(maddress);
}

void SyslogSink::record(LogRecord const& node)
{
    // write the header
    rewind(mbufferStream);
    format_time(mtimestamp, node.meta.time, 3, true);
    int bytesWritten = 0;
    int headerOffset;
    int priority = mfacility * 8 + std::max(std::min(7, node.meta.severity / 100), 0);
    if (muse_rfc3164 == false) {
        if (node.meta.tag[0]) {
            headerOffset = fprintf(mbufferStream, "<%d>1 %s %s %s - %s - ", priority, mtimestamp, mhostname,
                                   mapplication_name, node.meta.tag);
        } else {
            headerOffset =
                fprintf(mbufferStream, "<%d>1 %s %s %s - - - ", priority, mtimestamp, mhostname, mapplication_name);
        }
    } else {
        headerOffset = fprintf(mbufferStream, "<%d> %s %s %s: ", priority, mtimestamp, mhostname, mapplication_name);
    }
    bytesWritten += headerOffset;
    // Format the message into the buffer
    bytesWritten += mformat(mbufferStream, node);

    if (bytesWritten > kMaxDatagramSize) {
        // Truncate write
        bytesWritten = kMaxDatagramSize;
    }

    // Send the buffer
    if (msock_fd != -1) {
        int status;
        if (mtcp) {
            char sizeInfo[128];
            int sizeInfoSize = snprintf(sizeInfo, sizeof(sizeInfo), "%d ", bytesWritten);
            sendto(msock_fd, sizeInfo, sizeInfoSize, 0, maddress, maddress_size);
        }
        status = sendto(msock_fd, mbuffer, bytesWritten, 0, maddress, maddress_size);
        if (status == -1) { printf("Failed to send with code %d\n", errno); }
    }
    // printf("Echo log: %s\n", mdatagram_buffer);
    if (mecho) {
        fwrite(mbuffer + headerOffset, sizeof(char), bytesWritten - headerOffset, stdout);
        printf("\n");
    }
}

void SyslogSink::set_application_name(char const* application_name)
{
    strncpy(mapplication_name, application_name, sizeof(mapplication_name));
}

void SyslogSink::set_facility(int facility)
{
    mfacility = std::min(23, std::max(0, facility));
}

void SyslogSink::set_rfc3164_protocol(bool doit)
{
    muse_rfc3164 = doit;
}

}  // namespace slog
