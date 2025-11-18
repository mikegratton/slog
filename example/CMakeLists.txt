if (SLOG_STREAM_LOG)
    file(GLOB source CONFIGURE_DEPENDS *.cpp)

    add_executable(slogMinimal ${source})
    target_link_libraries(slogMinimal PUBLIC slog)

    if (SLOG_FORMAT_LOG)
        set_target_properties(slogMinimal PROPERTIES CXX_STANDARD 20)
    else()
        set_target_properties(slogMinimal PROPERTIES CXX_STANDARD 11)
    endif()
else()
    message(STATUS "Example requires SLOG_STREAM_LOG")
endif()