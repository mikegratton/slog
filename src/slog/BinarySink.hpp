#pragma once
#include "FileSink.hpp"

namespace slog {

/**
 * @brief Basic logging of binary data to files using the C api.
 *
 * The default format includes an 8 byte file header described in LogSink.hpp,
 * a 32 byte record header per message, and no file footer. These are all
 * configurable.
 *
 * Note that the "echo" function of the underlying file sink is not respected.
 *
 * LIMITATIONS: This sink records severity and size as four byte ints, limiting
 * the maximum record size to 4GB and the maximum severity to 2^31 - 1
 */
class BinarySink : public FileSink {
   public:
    BinarySink();
    ~BinarySink();
    BinarySink(BinarySink const&) = delete;
    BinarySink(BinarySink&&) noexcept = default;
    BinarySink& operator=(BinarySink const&) = delete;
    BinarySink& operator=(BinarySink&&) noexcept = default;

    /// Write the record to the file
    void record(LogRecord const& node) override;
};
}  // namespace slog