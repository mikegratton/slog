file(GLOB source CONFIGURE_DEPENDS *.cpp)

add_executable(benchmark ${source})
target_link_libraries(benchmark PUBLIC slog ${JOURNALD_LIBRARIES})
