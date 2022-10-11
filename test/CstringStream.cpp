#include "CaptureStream.hpp"
#include <ostream>
#include <iostream>
#include "doctest.h"

using namespace slog;

# if 0
TEST_CASE("BasicCStringStream")
{
    char buf[32];
    detail::get_stream(buf, 32) << "Hello, world. This is a very long message to write to a short buf";
    detail::terminate();
    bool term = false;
    for (int i=0; i<32; i++) {
        if (buf[i] == 0) {
            term = true;
            break;
        }
    }
    CHECK(term == true);
    
    detail::get_stream(buf, 32) << "Hello, world.";
    detail::terminate();
    term = false;
    for (int i=0; i<32; i++) {
        if (buf[i] == 0) {
            term = true;
            break;
        }
    }
    CHECK(term == true);
    
    detail::get_stream(buf, 32) << "Write1-" << "Write2" << "Write3";
    detail::terminate();
    term = false;
    for (int i=0; i<32; i++) {
        if (buf[i] == 0) {
            term = true;
            break;
        }
    }
    std::cout << buf << "\n";
    CHECK(term == true);
    CHECK(buf[0] == 'W');
}
#endif
