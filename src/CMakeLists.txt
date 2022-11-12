file(GLOB source CONFIGURE_DEPENDS *.cpp)

configure_file(LogConfig.hpp.in LogConfig.hpp)
find_package(Threads REQUIRED)
find_package(Journald)

if(NOT JOURNALD_FOUND)
    message("libsystemd-dev not found, journal log sink disabled")
    list(REMOVE source JournaldSink.cpp)
endif()

add_library(slog ${source})
set_target_properties(slog PROPERTIES CXX_STANDARD 11)
target_link_libraries(slog PUBLIC Threads::Threads)

if(JOURNALD_FOUND)
    target_link_libraries(slog PUBLIC Journald::Journald)
endif()

target_include_directories(slog PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>    
    $<INSTALL_INTERFACE:include/slog>
)

# Installation
include(GNUInstallDirs)
# Public headers
install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/LogConfig.hpp
    slog.hpp    
    LogRecord.hpp
    LogRecordPool.hpp
    LogSetup.hpp
    LogSink.hpp
    FileSink.hpp
    ConsoleSink.hpp
    $<$<BOOL:JOURNALD_FOUND>:${CMAKE_CURRENT_SOURCE_DIR}/JournaldSink.hpp>
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/slog
    )
# export(TARGETS slog NAMESPACE slog:: FILE SlogConfig.cmake)
install(TARGETS slog 
    EXPORT SlogConfig
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    )
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    SlogConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
    )
install(EXPORT SlogConfig
        FILE SlogConfig.cmake
        NAMESPACE slog::
        DESTINATION lib/cmake/slog
         )
