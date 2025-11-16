configure_file(LogConfig.hpp.in LogConfig.hpp)

set(SLOG_SOURCE
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
    Signal.cpp
    slog.cpp
    SlogError.cpp
    SyslogSink.cpp
    ThresholdMap.cpp
)

set(SLOG_PUBLIC_HEADERS
    slog.hpp
    slogDetail.hpp
    BinarySink.hpp
    ConsoleSink.hpp
    FileSink.hpp
    LogSetup.hpp
    LogRecord.hpp
    LogRecordPool.hpp
    LogSink.hpp
    ThresholdMap.hpp    
)


#########################################################################################
# PlatformUtilities

# Check for POSIX functions
include(CheckSymbolExists)
check_symbol_exists(realpath stdlib.h SLOG_HAVE_REALPATH)
check_symbol_exists(mkdir sys/stat.h SLOG_HAVE_MKDIR)
check_symbol_exists(gmtime_r time.h SLOG_HAVE_GMTIME_R)
check_symbol_exists(sigaction signal.h SLOG_HAVE_SIGACTION)
if (SLOG_HAVE_REALPATH AND SLOG_HAVE_MKDIR AND SLOG_HAVE_GMTIME_R AND SLOG_HAVE_SIGACTION)
    set(SLOG_PLATFORM POSIX)
endif()
if (${SLOG_PLATFORM} STREQUAL "POSIX")
    message(STATUS "POSIX PlatformUtilties selected")
    list(APPEND SLOG_SOURCE PlatformUtilitiesPosix.cpp)    
else()
    message(FATAL_ERROR "No viable PlatformUtilties are available for this platform")
endif()


#########################################################################################
# Extra sinks

# Journald
find_package(Journald)
if (JOURNALD_FOUND AND SLOG_JOURNALD)
    message(STATUS "Systemd journal log sink enabled")
    list(APPEND SLOG_SOURCE JournaldSink.cpp)
    list(APPEND SLOG_PUBLIC_HEADERS JournaldSink.hpp)    
else()    
    set(SLOG_JOURNALD OFF CACHE BOOL "Provide journald sink (requires systemd-dev to be installed)" FORCE)
    message(STATUS "Systemd journal log sink disabled.")    
endif()

# Syslog
include(CheckSymbolExists)
check_symbol_exists(gethostname unistd.h SLOG_HAVE_GETHOSTNAME)
check_symbol_exists(socket sys/socket.h SLOG_HAVE_SOCKET)
check_symbol_exists(getaddrinfo netdb.h SLOG_HAVE_GETADDRINFO)
if (SLOG_HAVE_GETHOSTNAME AND SLOG_HAVE_SOCKET AND SLOG_HAVE_GETADDRINFO)
    message(STATUS "Syslog log sink enabled")
    list(APPEND SLOG_SOURCE SyslogSink.cpp)    
    list(APPEND SLOG_PUBLIC_HEADERS SyslogSink.hpp)
else()
    message(STATUS "Syslog log sink disabled")
endif()


#########################################################################################
# Slog library

add_library(slog ${SLOG_SOURCE})
set_target_properties(slog PROPERTIES CXX_STANDARD 11)
target_include_directories(slog PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>    
)

find_package(Threads REQUIRED)
target_link_libraries(slog PUBLIC Threads::Threads)

if(SLOG_JOURNALD AND JOURNALD_FOUND)
    target_link_libraries(slog PUBLIC Journald::Journald)
endif()

# Warnings
target_compile_options(slog PRIVATE -Wall -Wextra)

# Static analysis 
add_custom_target(slogAnalysis
    COMMAND clang-tidy -p ${CMAKE_BINARY_DIR} ${SLOG_SOURCE}
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)


#############################################################
# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(SLOG_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/slog" CACHE PATH "Path where headers will be installed")
set(SLOG_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Path for libraries and cmake files")
set(SLOG_CONFIG_INSTALL_DIR ${SLOG_LIB_INSTALL_DIR}/cmake/slog)
mark_as_advanced(SLOG_INCLUDE_INSTALL_DIR SLOG_LIB_INSTALL_DIR)
set(PACKAGE_VERSION ${PROJECT_VERSION})

# Public headers
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LogConfig.hpp
        ${SLOG_PUBLIC_HEADERS}
    DESTINATION 
        ${SLOG_INCLUDE_INSTALL_DIR}
    COMPONENT 
        Development
)

# Targets
install(
    TARGETS
        slog
    EXPORT 
        SlogTargets    
    INCLUDES DESTINATION 
        ${CMAKE_INSTALL_INCLUDEDIR}
)

# CMake config/version/target files
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/SlogConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SlogConfig.cmake
    INSTALL_DESTINATION 
        ${SLOG_CONFIG_INSTALL_DIR}
    PATH_VARS 
        SLOG_INCLUDE_INSTALL_DIR
        SLOG_LIB_INSTALL_DIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SlogConfigVersion.cmake
    VERSION 
        ${PACKAGE_VERSION}
    COMPATIBILITY 
        SameMajorVersion
)
install(
    FILES 
        ${CMAKE_CURRENT_BINARY_DIR}/SlogConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SlogConfigVersion.cmake
        ${PROJECT_SOURCE_DIR}/cmake/FindJournald.cmake
    DESTINATION 
        ${SLOG_CONFIG_INSTALL_DIR}
)
install(
    EXPORT
        SlogTargets
    NAMESPACE
        slog::
    DESTINATION
        ${SLOG_CONFIG_INSTALL_DIR}
)
