#include <cstdarg>
#include <cstdio>

#include "LogRecordPool.hpp"  // For RecordNode
#include "slog.hpp"

namespace slog {

/// printf-style record capture
RecordNode* capture_message(RecordNode* node, char const* format_, ...)
{
    if (node) {
        va_list vlist;
        va_start(vlist, format_);
        node->rec.message_byte_count = vsnprintf(node->rec.message, node->rec.message_max_size, format_, vlist) + 1;
        va_end(vlist);
    }
    return node;
}

}  // namespace slog
