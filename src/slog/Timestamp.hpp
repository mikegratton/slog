#pragma once
#include <cstdint>

namespace slog
{

/**
 * A timestamp class, with the ability to format to ISO-8601 strings
 */
class Timestamp
{
  public:
    enum TimeFormatMode {
        FULL_SPACE, // YYYY-MM-DD hh:mm:ss.fZ
        COMPACT,    // YYYYMMDDThhmmss.fZ
        FULL_T      // YYYY-MM-DDThh:mm:ss.fZ
    };

    /// Make a timestamp with a given offset from the epoch
    constexpr Timestamp(uint64_t i_nanosecondsSinceEpoch = 0)
        : m_nanosSinceEpoch(i_nanosecondsSinceEpoch)
    {
    }

    /// Make a timestamp representing now according to the system clock
    static Timestamp now();

    /// Get the offset from epoch
    constexpr uint64_t nanoseconds_since_epoch() const { return m_nanosSinceEpoch; }

    /**
     * @brief Write an ISO 8601 timestamp to a string
     *
     * Format the time in ISO 8601/RFC 3339 format YYYY-MM-DD hh:mm:ss.fZ
     * where
     *  (1) The timezone is always UTC
     *  (2) The number of fractional second digits f is controlled by
     *      seconds_decimal_precision
     * @note The string should be at least 21 characters long for zero fractional seconds.
     * For the default form, 25 characters are required.
     */
    void format_time(char* time_str, int seconds_precision = 3, TimeFormatMode format = FULL_SPACE) const;

  private:
    uint64_t m_nanosSinceEpoch;
};

} // namespace slog