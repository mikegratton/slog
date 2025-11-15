cmake_minimum_required(VERSION 3.16)
project(slog 
    VERSION 2.0.0.0
    DESCRIPTION 
        "Asynchronous logging for C++"
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(SLOG_STREAM "Provide Slog() macros and include <iostream>" ON)
option(SLOG_BINARY_LOG "Provide Blog() binary data logging macros" OFF)
option(SLOG_JOURNALD "Provide journald sink (requires systemd-dev to be installed)" ON)
option(SLOG_LOG_TO_CONSOLE_WHEN_STOPPED "When slog is stopped, print messages to the console instead of suppressing them" OFF)
option(SLOG_PRINT_ERROR "Print internal errors to stderr" ON)
set(SLOG_DEFAULT_RECORD_SIZE 512 CACHE STRING "Size in bytes of default record")
set(SLOG_DEFAULT_POOL_RECORD_COUNT 256 CACHE STRING "Number of records to allocate")

# Sanitizer builds
set(SLOG_SANITIZER_BUILD "none" CACHE STRING "Build with Google sanitizers")
set_property(CACHE SLOG_SANITIZER_BUILD PROPERTY STRINGS "none" "address" "thread")
if (${SLOG_SANITIZER_BUILD} STREQUAL "none")
    # 
elseif (${SLOG_SANITIZER_BUILD} STREQUAL "address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined -ggdb -fno-omit-frame-pointer")
elseif (${SLOG_SANITIZER_BUILD} STREQUAL "thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -ggdb -fno-omit-frame-pointer")
else()
    message(STATUS "SLOG_SANITIZER_BUILD setting of \"${SLOG_SANITIZER_BUILD}\" is not known")
endif()

# Core library
add_subdirectory(src)

# Examples and tests
option(SLOG_BUILD_TEST "Compile unit tests" OFF)
option(SLOG_BUILD_EXAMPLE "Compile examples" OFF)
option(SLOG_BUILD_BENCHMARK "Compile benchmarks" OFF)
if (SLOG_BUILD_TEST)
    add_subdirectory(test)
endif()
if (SLOG_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()
if(SLOG_BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

# Installation
install(TARGETS slog
    EXPORT SlogTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    )
 
install(EXPORT SlogTargets 
    NAMESPACE slog::
    DESTINATION lib/cmake
    COMPONENT Development
    )
