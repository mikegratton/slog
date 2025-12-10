#pragma once
#include "SlogConfig.hpp"
#include <cstdarg>

namespace slog
{
/// Print internal slog errors to stderr if SLOG_PRINT_ERROR is defined to a
/// nonzero value.
void slog_error(char const* format, ...);

/// Actually print errors to stderr
void slog_print_error(char const* format, va_list args);

///////////////////////////////////////////////////////////////////////

inline void slog_error(char const* format, ...)
{
#if SLOG_PRINT_ERROR
    va_list vlist;
    va_start(vlist, format);
    slog_print_error(format, vlist);
    va_end(vlist);
#endif
}

} // namespace slog