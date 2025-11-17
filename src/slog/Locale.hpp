#pragma once
#include <locale>

namespace slog
{

/**
 * Set the slog locale for all streams and formats
 */ 
void set_locale(std::locale const& locale);

/**
 * Get the slog locale for all streams and formats
 */ 
std::locale const& get_locale();

/**
 * Get the slog locale version (used for updating streams)
 */ 
int get_locale_version();

/**
 * Set the slog locale to the global locale
 */ 
void set_locale_to_global();

} // namespace slog