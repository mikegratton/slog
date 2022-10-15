#include "CapturePrintf.hpp"
#include <cstdarg>
#include <cstdio>
#include "LogRecordPool.hpp" // For RecordNode

namespace slog {
    
RecordNode* capture_message(RecordNode* node, char const* format_, ...)
{
    va_list vlist;
    va_start(vlist, format_);
    vsnprintf(node->rec.message, node->rec.message_max_size, format_, vlist);
    va_end(vlist);
    return node;
}

}
