#pragma once
#include "LogCore.hpp"
#include <iosfwd>

namespace slog {

// A special ostream wrapper that writes to node's message buffer.
// On destruction, the node is pushed to the back end
class CaptureStream {
public:
    CaptureStream(RecordNode* node_) : node(node_) { }

    ~CaptureStream();

    std::ostream& stream();

protected:
    RecordNode* node;
};
}
