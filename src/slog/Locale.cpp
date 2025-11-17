#include "Locale.hpp"
#include <locale>

namespace slog {
namespace
{
/// Global locale for slog
std::locale static g_locale;

/// Tracking of the current locale
int static s_locale_version = 0;
} // namespace

void set_locale(std::locale const& loc)
{
    g_locale = loc;
    s_locale_version++;
}

std::locale const& get_locale()
{
    return g_locale;
}

int get_locale_version()
{
    return s_locale_version;
}

void set_locale_to_global()
{
    set_locale(std::locale(""));
}

} // namespace slog
