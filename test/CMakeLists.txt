file(GLOB source CONFIGURE_DEPENDS *.cpp)

if (NOT SLOG_JOURNALD)
    list(FILTER source EXCLUDE REGEX ".*Journald.*")
endif()


add_executable(slogTest ${source})
target_link_libraries(slogTest PUBLIC slog)

