file(GLOB source CONFIGURE_DEPENDS *.cpp)

add_executable(clogTest ${source})
target_link_libraries(clogTest PUBLIC clog)

