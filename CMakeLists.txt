cmake_minimum_required(VERSION 3.12)
project(slog VERSION 0.1.0.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(SLOG_ALWAYS_LOG "When slog is stopped, write all messages to the console" OFF)
option(SLOG_LOCK_FREE_POOL "[EXPERIMENTAL] Use the lock free message pool" OFF)
option(SLOG_POOL_BLOCKS_WHEN_EMPTY "When the pool of available message is empty, block logging until available" ON)
set(SLOG_MAX_CHANNEL 4 CACHE STRING "Total number of log channels to provide")
set(SLOG_MESSAGE_SIZE 1024 CACHE STRING "Maximum size in bytes of a log message")
set(SLOG_POOL_SIZE 1048576 CACHE STRING "Bytes allocated per channel")
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local CACHE PATH "Installation prefix" FORCE)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE PATH "Build type" FORCE)
endif()
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR “In-source build detected!”)
endif()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(benchmark)

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
