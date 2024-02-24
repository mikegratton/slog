cmake_minimum_required(VERSION 3.12)
project(slog VERSION 1.2.0.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(SLOG_LOG_TO_CONSOLE_WHEN_STOPPED "When slog is stopped, write all messages to the console" OFF)
option(SLOG_STREAM "Provide Slog() macros and include <iostream>" ON)
set(SLOG_DEFAULT_RECORD_SIZE 512 CACHE STRING "Size in bytes of default record")
set(SLOG_DEFAULT_POOL_RECORD_COUNT 2048 CACHE STRING "Number of records to allocate")

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)
add_subdirectory(benchmark)

set_target_properties(slogTest slogMinimal benchmark PROPERTIES EXCLUDE_FROM_ALL TRUE)

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
