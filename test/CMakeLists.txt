file(GLOB source CONFIGURE_DEPENDS *.cpp)

set(source
    Basics.cpp
    BinaryLogTest.cpp
    FileLogTest.cpp
    FlogTest.cpp
    HandlerTest.cpp
    JumboTest.cpp
    main.cpp
    PlatformUtilitiesTest.cpp
    SlowSink.cpp
    StoppedTest.cpp
    testUtilities.cpp
    ThresholdMapTest.cpp
)

if (SLOG_JOURNALD)
    list(APPEND source JournaldTest.cpp)
endif()
if (SLOG_SYSLOG)
    list(APPEND source SyslogTest.cpp)
endif()


add_executable(slogTest ${source})
target_link_libraries(slogTest PUBLIC slog)
target_compile_definitions(slogTest 
    PUBLIC 
        -DDOCTEST_CONFIG_INCLUDE_TYPE_TRAITS 
        -DDOCTEST_CONFIG_SUPER_FAST_ASSERTS
)

add_executable(HandlerProgram HandlerProgram.cpp SlowSink.cpp)
target_link_libraries(HandlerProgram PUBLIC slog)
