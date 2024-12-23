#include "FileSink.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <limits>

#include "slog/LogSink.hpp"

namespace slog {

FileSink::FileSink()
{
    mfile = nullptr;
    format_time(msessionStartTime, std::chrono::system_clock::now().time_since_epoch().count(), 0, COMPACT);
    set_max_file_size(std::numeric_limits<long>::max() - 2048);
    set_echo(true);
    set_file(".", program_invocation_short_name);
    set_formatter(default_format);
}

FileSink::~FileSink()
{
    if (mfile) {
        mfooter(mfile, msequence, std::chrono::system_clock::now().time_since_epoch().count());
        fclose(mfile);
    }
}

void FileSink::set_max_file_size(long isize)
{
    mmaxBytes = isize;
    msequence = 0;
    mbytesWritten = 0;
}

void FileSink::set_file(char const* location, char const* name, char const* end)
{
    strncpy(mfileLocation, location, sizeof(mfileLocation));
    strncpy(mfileName, name, sizeof(mfileName));
    strncpy(mfileEnd, end, sizeof(mfileEnd));
    if (mfile) { fclose(mfile); }
    mfile = nullptr;
}

void FileSink::open_or_rotate()
{
    if (mbytesWritten > mmaxBytes) {
        if (mfile) {
            mfooter(mfile, msequence, std::chrono::system_clock::now().time_since_epoch().count());
            fclose(mfile);
        }
        mfile = nullptr;
        mbytesWritten = 0;
    }

    char fname[sizeof(mfileLocation) + sizeof(mfileName) + sizeof(mfileEnd) + sizeof(msessionStartTime) + 8];
    snprintf(fname, sizeof(fname), "%s/%s_%s_%03d.%s", mfileLocation, mfileName, msessionStartTime, msequence,
             mfileEnd);
    mfile = fopen(fname, "w");

    if (mfile == nullptr) {
        fprintf(stderr, "Could not open log file %s\n", fname);
        return;
    }
    mheader(mfile, msequence, std::chrono::system_clock::now().time_since_epoch().count());
    msequence++;
}

void FileSink::record(LogRecord const& rec)
{
    open_or_rotate();
    if (nullptr == mfile) { return; }
    mbytesWritten += mformat(mfile, rec);
    fputc('\n', mfile);
    fflush(mfile);
    if (mecho) {
        mformat(stdout, rec);
        fputc('\n', stdout);
        fflush(stdout);
    }
}

void FileSink::set_file_header_format(LogFileFurniture headerFormat)
{
    mheader = headerFormat;
}

void FileSink::set_file_footer_format(LogFileFurniture footerFormat)
{
    mfooter = footerFormat;
}

}  // namespace slog
