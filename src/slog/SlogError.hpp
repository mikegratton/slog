#pragma once
#include "LogConfig.hpp"
#include <cstdarg>

namespace slog
{
/// Print internal slog errors to stderr if SLOG_PRINT_ERROR is defined to a
/// nonzero value.
void slog_error(char const* format_, ...);

/// Print errors to stderr
void slog_print_error(char const* format_, va_list args);

///////////////////////////////////////////////////////////////////////

inline void slog_error(char const* format_, ...)
{
#if SLOG_PRINT_ERROR
    va_list vlist;
    va_start(vlist, format_);
    slog_print_error(format_, vlist);
    va_end(vlist);
#endif
}

} // namespace slog