#include "BinarySink.hpp"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>

#include "slog/LogRecord.hpp"
#include "slog/LogSink.hpp"

namespace slog {

BinarySink::BinarySink()
{
    mfile = nullptr;
    format_time(msessionStartTime, std::chrono::system_clock::now().time_since_epoch().count(), 0, COMPACT);
    set_max_file_size(std::numeric_limits<long>::max() - 2048);
    set_file(".", program_invocation_short_name);
}

BinarySink::~BinarySink()
{
    if (mfile) { fclose(mfile); }
}

void BinarySink::set_max_file_size(long isize)
{
    mmaxBytes = isize;
    msequence = 0;
    mbytesWritten = 0;
}

void BinarySink::set_file(char const* location, char const* name, char const* end)
{
    strncpy(mfileLocation, location, sizeof(mfileLocation));
    strncpy(mfileName, name, sizeof(mfileName));
    strncpy(mfileEnd, end, sizeof(mfileEnd));
    if (mfile) { fclose(mfile); }
    mfile = nullptr;
}

void BinarySink::open_or_rotate()
{
    if (mbytesWritten > mmaxBytes) {
        if (mfile) { fclose(mfile); }
        mfile = nullptr;
        mbytesWritten = 0;
    }

    if (mfile == nullptr) {
        char fname[sizeof(mfileLocation) + sizeof(mfileName) + sizeof(mfileEnd) + sizeof(msessionStartTime) + 8];
        snprintf(fname, sizeof(fname), "%s/%s_%s_%03d.%s", mfileLocation, mfileName, msessionStartTime, msequence,
                 mfileEnd);
        msequence++;
        mfile = fopen(fname, "w");
    }
}

void BinarySink::record(LogRecord const& rec)
{
    open_or_rotate();
    // write leader here
    uint32_t reducedSize = 0;
    int32_t reducedSev = static_cast<int32_t>(rec.meta.severity);
    LogRecord const* r = &rec;
    do {
        reducedSize += static_cast<uint32_t>(rec.message_byte_count);
        r = r->more;
    } while (r);
    mbytesWritten += fwrite(&reducedSize, sizeof(uint32_t), 1, mfile);
    mbytesWritten += fwrite(&reducedSev, sizeof(int32_t), 1, mfile);
    mbytesWritten += fwrite(&rec.meta.time, sizeof(unsigned long), 1, mfile);
    mbytesWritten += fwrite(rec.meta.tag, sizeof(char), TAG_SIZE, mfile);

    r = &rec;
    do {
        mbytesWritten += fwrite(r->message, 1, r->message_byte_count, mfile);
        r = r->more;
    } while (r);
    fflush(mfile);
}

}  // namespace slog
