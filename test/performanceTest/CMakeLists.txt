add_executable(performanceTest 
    LogPerformance.cpp
    LogStreamPerformance.cpp
    StatsSink.cpp
    main.cpp
)

target_link_libraries(performanceTest PUBLIC slog)