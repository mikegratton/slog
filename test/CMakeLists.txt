file(GLOB source CONFIGURE_DEPENDS *.cpp)

if (NOT SLOG_JOURNALD)
    list(FILTER source EXCLUDE REGEX ".*Journald.*")
endif()
if (NOT SLOG_SYSLOG)
    list(FILTER source EXCLUDE REGEX ".*Syslog.*")
endif()


add_executable(slogTest ${source})
target_link_libraries(slogTest PUBLIC slog)

add_subdirectory(signalTest)
add_subdirectory(exitTest)
add_subdirectory(performanceTest)

add_subdirectory(udpTest)