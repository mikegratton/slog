#pragma once
#include <ctime>

namespace slog
{

/**
 * @brief Get the name of this program
 */
char const* program_short_name();

/**
 * @brief Transform seconds from 1970-01-01 00:00:00Z to a tm struct
 */
void zulu_time_r(struct tm* o_datetime, time_t const* seconds_since_epoch);

/**
 * @brief Create the directory, including needed parent directories (like mkdir -p)
 *
 * This will return true if the directory is a valid path, and false if directories
 * can't be created.
 */
bool make_directory(char const* directory, int mode = 509);

/**
 * Install this handler for all slog-handled signals
 */
typedef void (*signal_handler)(int);
void install_signal_handlers(signal_handler handler);

/**
 * Remove the slog signal handler and reinstall the old handlers
 */
void restore_old_signal_handlers();

/**
 * Forward the signal to the old handler we replaced. If we didn't
 * install a handler for signal_id, do nothing
 */
void forward_signal(int signal_id);
} // namespace slog