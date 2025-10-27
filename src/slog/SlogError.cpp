#include "SlogError.hpp"
#include <cstdarg>
#include <cstdio>

namespace slog
{

void slog_print_error(char const* format_, va_list args)
{
    fprintf(stderr, "Slog: ");
    vfprintf(stderr, format_, args);
}
} // namespace slog