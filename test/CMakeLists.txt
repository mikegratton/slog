file(GLOB source CONFIGURE_DEPENDS *.cpp)
add_executable(slogTest ${source})
target_link_libraries(slogTest PUBLIC slog)

