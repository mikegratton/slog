#include "BinarySink.hpp"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>

#include "slog/FileSink.hpp"
#include "slog/LogRecord.hpp"
#include "slog/LogSink.hpp"

namespace slog {

BinarySink::BinarySink() : FileSink()
{
    mformat = default_binary_format;
    mheader = default_binary_header_furniture;
    mfooter = no_op_furniture;
}

BinarySink::~BinarySink()
{
    // Nothing extra here
}

void BinarySink::record(LogRecord const& rec)
{
    open_or_rotate();
    if (nullptr == mfile) { return; }
    mbytesWritten += mformat(mfile, rec);
    fflush(mfile);
}

}  // namespace slog
