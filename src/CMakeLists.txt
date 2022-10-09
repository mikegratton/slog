find_package(Threads REQUIRED)

file(GLOB source CONFIGURE_DEPENDS *.cpp)

add_library(clog STATIC ${source} )
target_link_libraries(clog PUBLIC Threads::Threads)
target_include_directories(clog PUBLIC ${CMAKE_CURRENT_LIST_DIR})
