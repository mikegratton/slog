file(GLOB source CONFIGURE_DEPENDS *.cpp)
add_compile_options(-pg)

add_executable(slogBenchmark ${source})
target_link_libraries(slogBenchmark PUBLIC slog ${JOURNALD_LIBRARIES})
