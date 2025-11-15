#include "BinarySink.hpp"
#include "slog/LogRecord.hpp"
#include "slog/LogSink.hpp"

namespace slog {

BinarySink::BinarySink() : FileSink()
{
    mformat = long_binary_format;
    mheader = default_binary_header_furniture;
    mfooter = no_op_furniture;
}

BinarySink::~BinarySink() = default;

void BinarySink::record(LogRecord const& rec)
{
    open_or_rotate();
    if (nullptr == mfile) { return; }
    mbytesWritten += mformat(mfile, rec);
    fflush(mfile);
}

}  // namespace slog
