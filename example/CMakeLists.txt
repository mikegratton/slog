file(GLOB source CONFIGURE_DEPENDS *.cpp)

add_executable(slogMinimal ${source})
target_link_libraries(slogMinimal PUBLIC slog)

