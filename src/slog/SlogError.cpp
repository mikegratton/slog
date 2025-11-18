#include "SlogError.hpp"
#include <cstdarg>
#include <cstdio>

namespace slog
{

void slog_print_error(char const* format, va_list args)
{
    fprintf(stderr, "Slog: ");
    vfprintf(stderr, format, args);
}
} // namespace slog