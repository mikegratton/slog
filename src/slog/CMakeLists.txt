include(CheckSymbolExists)

configure_file(LogConfig.hpp.in LogConfig.hpp)

set(source
    BinarySink.cpp
    CaptureBinary.cpp    
    CaptureStream.cpp
    FileSink.cpp
    LogChannel.cpp
    LoggerSingleton.cpp
    LogRecord.cpp
    LogRecordPool.cpp
    LogSetup.cpp
    LogSink.cpp    
    slog.cpp
    SlogError.cpp
    SyslogSink.cpp
    ThresholdMap.cpp
)


#########################################################################################
# PlatformUtilities 
# Check for POSIX functions
check_symbol_exists(realpath stdlib.h SLOG_HAVE_REALPATH)
check_symbol_exists(mkdir sys/stat.h SLOG_HAVE_MKDIR)
check_symbol_exists(gmtime_r time.h SLOG_HAVE_GMTIME_R)
check_symbol_exists(sigaction signal.h SLOG_HAVE_SIGACTION)
if (SLOG_HAVE_REALPATH AND SLOG_HAVE_MKDIR AND SLOG_HAVE_GMTIME_R AND SLOG_HAVE_SIGACTION)
    set(SLOG_POSIX On)
endif()
mark_as_advanced(SLOG_POSIX)

if (SLOG_POSIX)
    message(STATUS "POSIX PlatformUtilties selected")
    list(APPEND source PlatformUtilitiesPosix.cpp)
else()
    message(FATAL_ERROR "No viable PlatformUtilties are available for this platform")
endif()


#########################################################################################
# Extra sinks
find_package(Journald)
if (JOURNALD_FOUND AND SLOG_JOURNALD)
    list(APPEND source JournaldSink.cpp)
    message(STATUS "Systemd journal log sink enabled")
else()    
    set(SLOG_JOURNALD OFF CACHE BOOL "Provide journald sink (requires systemd-dev to be installed)" FORCE)
    message(STATUS "Systemd journal log sink disabled.")    
endif()

check_symbol_exists(gethostname unistd.h SLOG_HAVE_GETHOSTNAME)
check_symbol_exists(socket sys/socket.h SLOG_HAVE_SOCKET)
check_symbol_exists(getaddrinfo netdb.h SLOG_HAVE_GETADDRINFO)
if (SLOG_HAVE_GETHOSTNAME AND SLOG_HAVE_SOCKET AND SLOG_HAVE_GETADDRINFO)
    list(APPEND source SyslogSink.cpp)
    message(STATUS "Syslog log sink enabled")
    set(SLOG_SYSLOG ON CACHE BOOL "Provide a syslog sink" FORCE)
else()
    message(STATUS "Syslog log sink disabled")
    set(SLOG_SYSLOG OFF CACHE BOOL "Provide a syslog sink" FORCE)
endif()


#########################################################################################
find_package(Threads REQUIRED)

add_library(slog ${source})
set_target_properties(slog PROPERTIES CXX_STANDARD 11)
target_link_libraries(slog PUBLIC Threads::Threads)

if(SLOG_JOURNALD AND JOURNALD_FOUND)
    target_link_libraries(slog PUBLIC Journald::Journald)
endif()

target_include_directories(slog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/slog>
)

# Installation
include(GNUInstallDirs)

# Public headers
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LogConfig.hpp
    slog.hpp
    LogRecord.hpp
    LogRecordPool.hpp
    LogSetup.hpp
    LogSink.hpp
    FileSink.hpp
    ConsoleSink.hpp
    SyslogSink.hpp
    $<$<BOOL:SLOG_SYSLOG>:${CMAKE_CURRENT_SOURCE_DIR}/SyslogSink.hpp>
    $<$<BOOL:SLOG_JOURNALD>:${CMAKE_CURRENT_SOURCE_DIR}/JournaldSink.hpp>
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/slog
)

# export(TARGETS slog NAMESPACE slog:: FILE SlogConfig.cmake)
install(TARGETS slog
    EXPORT SlogConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    SlogConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(EXPORT SlogConfig
        FILE SlogConfig.cmake
        NAMESPACE slog::
        DESTINATION lib/cmake/slog
)
