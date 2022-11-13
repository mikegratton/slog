cmake_minimum_required(VERSION 3.12)
project(slog VERSION 0.1.0.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

option(SLOG_LOG_TO_CONSOLE_WHEN_STOPPED "When slog is stopped, write all messages to the console" OFF)
option(SLOG_STREAM "Provide Slog() macros and include <iostream>" ON)

add_compile_options(-pg)

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
