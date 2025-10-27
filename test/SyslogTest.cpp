#include "doctest.h"
#include "slog/LogSetup.hpp"
#include "slog/PlatformUtilities.hpp"
#include "slog/SyslogSink.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

using namespace slog;

namespace
{
char const* socketPath = "/tmp/slogtest.sock";
}

TEST_CASE("Syslog.Rfc3164")
{
    sockaddr_un address{0};
    strncpy(address.sun_path, socketPath, sizeof(address.sun_path) - 1);
    address.sun_family = AF_UNIX;
    auto* caddress = reinterpret_cast<sockaddr*>(&address);
    auto address_size = sizeof(sockaddr_un);
    int sock_fd = sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    REQUIRE(0 == bind(sock_fd, caddress, address_size));

    auto sink = std::make_shared<slog::SyslogSink>(socketPath);
    sink->set_rfc3164_protocol(true);
    sink->set_echo(false);
    slog::LogConfig config;
    config.set_default_threshold(slog::DBUG);
    config.set_sink(sink);
    slog::start_logger(config);
    Slog(NOTE, "tag") << "Testing!";

    char buffer[65507];
    std::size_t count = recv(sock_fd, buffer, sizeof(buffer), 0);
    char const* leader = "<13> slogTest: [NOTE tag ";
    std::size_t leader_size = strlen(leader);
    std::size_t time_size = 23;
    CHECK(strncmp(buffer, leader, leader_size) == 0);
    CHECK(strcmp(buffer + leader_size + time_size, "] Testing!") == 0);

    shutdown(sock_fd, 0);
    std::remove(socketPath);
}

TEST_CASE("Syslog.Unix")
{
    sockaddr_un address{0};
    strncpy(address.sun_path, socketPath, sizeof(address.sun_path) - 1);
    address.sun_family = AF_UNIX;
    auto* caddress = reinterpret_cast<sockaddr*>(&address);
    auto address_size = sizeof(sockaddr_un);
    int sock_fd = sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    REQUIRE(0 == bind(sock_fd, caddress, address_size));

    auto sink = std::make_shared<slog::SyslogSink>(socketPath);
    sink->set_echo(false);
    sink->set_rfc3164_protocol(false);
    slog::LogConfig config;
    config.set_default_threshold(slog::DBUG);
    config.set_sink(sink);
    slog::start_logger(config);
    Slog(NOTE, "tag") << "Testing!";

    // Note: regardless, for a unix socket we expect rfc 3164 protocol
    char buffer[65507];
    std::size_t count = recv(sock_fd, buffer, sizeof(buffer), 0);
    char const* leader = "<13> slogTest: [NOTE tag ";
    std::size_t leader_size = strlen(leader);
    std::size_t time_size = 23;
    CHECK(strncmp(buffer, leader, leader_size) == 0);
    CHECK(strcmp(buffer + leader_size + time_size, "] Testing!") == 0);

    stop_logger();
    shutdown(sock_fd, 0);
    std::remove(socketPath);
}

TEST_CASE("Syslog.Udp")
{
    pid_t childPid = fork();
    if (childPid != 0) {
        int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        REQUIRE(sock_fd != 0);
        sockaddr_in address{0};
        address.sin_family = AF_INET;
        address.sin_port = htons(50514);
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        auto* caddress = reinterpret_cast<sockaddr*>(&address);
        auto address_size = sizeof(sockaddr_in);
        REQUIRE(0 == bind(sock_fd, caddress, address_size));
        char buffer[65507];
        std::size_t count = recvfrom(sock_fd, buffer, sizeof(buffer), 0, nullptr, nullptr);

        std::istringstream stream(buffer);
        std::string token;
        stream >> token;
        CHECK(token == "<13>1");
        stream >> token;
        CHECK(token.size() == 23);
        stream >> token;
        gethostname(buffer, sizeof(buffer) - 1);
        CHECK(token == buffer);
        stream >> token;
        CHECK(token == slog::program_short_name());
        stream >> token;
        CHECK(token == "-");
        stream >> token;
        CHECK(token == "-");
        stream >> token;
        CHECK(token == "-");
        stream >> token;
        CHECK(token == "[NOTE");
        stream >> token;
        stream >> token;
        CHECK(token == "Testing!");
        close(sock_fd);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto sink = std::make_shared<slog::SyslogSink>("127.0.0.1:50514", false);
        sink->set_rfc3164_protocol(false);
        sink->set_echo(false);
        slog::LogConfig config(slog::DBUG, sink);
        slog::start_logger(config);
        Slog(NOTE) << "Testing!";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stop_logger();
        exit(0);
    }
}

TEST_CASE("Syslog.Tcp")
{
    stop_logger();

    pid_t childPid = fork();
    if (childPid != 0) {
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{0};
        address.sin_family = AF_INET;
        address.sin_port = htons(50514);
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        auto* caddress = reinterpret_cast<sockaddr*>(&address);
        auto address_size = sizeof(sockaddr_in);
        int code = bind(sock_fd, caddress, address_size);                
        if (0 != code) {
            perror(nullptr);
            REQUIRE(0 == code);
        }
        printf("Waiting for connection\n");
        REQUIRE(0 == listen(sock_fd, 1));
        socklen_t adsize2;
        int cfd = accept(sock_fd, caddress, &adsize2);
        perror(nullptr);        
        CHECK(ntohs(address.sin_port) == 50514);
        CHECK(ntohl(address.sin_addr.s_addr) ==  0x7f000001);
        REQUIRE(0 < cfd);
        char buffer[65507];
        std::size_t rec = 0;
        
        // Look for the octet count
        char* cursor = buffer;
        while(*cursor != ' ') {
            cursor += read(cfd, buffer, 1);
        }        
        *cursor = '\0';
        std::size_t expectedBytes = atoi(buffer);
        printf("Count: %ld\n", expectedBytes);

        // Read the expected bytes
        REQUIRE(expectedBytes == read(cfd, buffer, expectedBytes));   
        CHECK(expectedBytes == 89);     
        printf("Got %ld bytes: %s\n", expectedBytes, buffer);
        shutdown(cfd, SHUT_RDWR);
        close(cfd);
        close(sock_fd);
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        printf("Try to connect\n");
        auto sink = std::make_shared<slog::SyslogSink>("localhost:50514", true);
        sink->set_echo(true);
        slog::LogConfig config(slog::DBUG, sink);        
        slog::start_logger(config);
        Slog(NOTE) << "Testing!";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stop_logger();
        exit(0);
    }
}