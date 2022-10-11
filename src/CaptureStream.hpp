#pragma once
#include "LogCore.hpp"
#include <iosfwd>

namespace slog {

class CaptureStream {
public:
    CaptureStream(LogRecord* rec_) : rec(rec_) { }

    ~CaptureStream();

    std::ostream& stream();

protected:
    LogRecord* rec;
};
}
