file(GLOB source CONFIGURE_DEPENDS *.cpp)

if (NOT SLOG_JOURNALD)
    list(FILTER source EXCLUDE REGEX ".*Journald.*")
endif()
if (NOT SLOG_SYSLOG)
    list(FILTER source EXCLUDE REGEX ".*Syslog.*")
endif()


add_executable(slogTest ${source})
target_link_libraries(slogTest PUBLIC slog)
target_compile_definitions(slogTest 
    PUBLIC 
        -DDOCTEST_CONFIG_INCLUDE_TYPE_TRAITS 
        -DDOCTEST_CONFIG_SUPER_FAST_ASSERTS
)

add_subdirectory(signalTest)
add_subdirectory(performanceTest)
