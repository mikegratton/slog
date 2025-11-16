#include "BinarySink.hpp"
#include "slog/LogRecord.hpp"
#include "slog/LogSink.hpp"

namespace slog {

BinarySink::BinarySink() : FileSink()
{
    set_formatter(default_binary_format);
    set_file_header_format(default_binary_header_furniture);
    set_file_footer_format(no_op_furniture);
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
