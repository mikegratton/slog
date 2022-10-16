cmake_minimum_required(VERSION 3.12)
project(slog VERSION 0.0.0.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(Journald REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(minimal)
