add_executable(signalTest main.cpp slowSink.cpp)
target_link_libraries(signalTest PUBLIC slog)