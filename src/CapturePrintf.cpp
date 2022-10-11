#include "CapturePrintf.hpp"
#include <cstdarg>
#include <cstdio>

namespace slog {
    
LogRecord* capture_message(LogRecord* rec, char const* format_, ...)
{
    va_list vlist;
    va_start(vlist, format_);
    vsnprintf(rec->message, rec->message_max_size, format_, vlist);
    va_end(vlist);
    return rec;
}

}
