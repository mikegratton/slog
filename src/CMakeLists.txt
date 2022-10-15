find_package(Threads REQUIRED)

file(GLOB source CONFIGURE_DEPENDS *.cpp)

add_library(slog STATIC ${source} )

target_link_libraries(slog PUBLIC Threads::Threads atomic)
target_include_directories(slog PUBLIC ${CMAKE_CURRENT_LIST_DIR})

include(GNUInstallDirs)

install(FILES 
    slog.hpp
    LogConfig.hpp
    LogRecord.hpp
    LogSetup.hpp
    LogSink.hpp
    FileSink.hpp
    SinkTools.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/slog
    )
