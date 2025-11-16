#include "FileSink.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <limits>

#include "LogSink.hpp"
#include "PlatformUtilities.hpp"

namespace slog {

FileSink::FileSink()
: mfile(nullptr)
, mformat(default_format)
, mheader(no_op_furniture)
, mfooter(no_op_furniture)
, msessionStartTime(std::chrono::system_clock::now().time_since_epoch().count())
, msequence(0)
, mbytesWritten(0)
, mmaxBytes(std::numeric_limits<long>::max())
, mecho(true) 
, mfullLogName{""}   
{        
    format_time(msessionStartTimeStr, msessionStartTime, 0, COMPACT);
    set_max_file_size(std::numeric_limits<long>::max() - 2048);    
    set_file(".", ::slog::program_short_name());    
}

FileSink::~FileSink()
{
    close_file();
}

void FileSink::finalize()
{
    close_file();
}

void FileSink::close_file()
{
    if (mfile) {
        mfooter(mfile, msequence, std::chrono::system_clock::now().time_since_epoch().count());
        fclose(mfile);
        mfile = nullptr;
        mbytesWritten = 0;
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
    strncpy(mlogDirectory, location, sizeof(mlogDirectory) - 1);
    strncpy(mlogBaseName, name, sizeof(mlogBaseName) - 1);
    strncpy(mlogExtension, end, sizeof(mlogExtension) - 1);
    make_file_name();

    // Try to open the file
    if (!make_directory(mlogDirectory, 0777)) {
        fprintf(stderr, "Slog FileSink cannot continue\n");           
    } 

    if (mfile) { fclose(mfile); }
    mfile = nullptr;
}

void FileSink::open_or_rotate()
{    
    if (mbytesWritten > mmaxBytes) {
        close_file();
    }
    if (!mfile) {
        make_file_name();
        mfile = fopen(mfullLogName, "w");
        if (mfile == nullptr) {
            fprintf(stderr, "Slog: could not open log file %s\n", mfullLogName);            
            return;
        }
        mheader(mfile, msequence,
                std::chrono::system_clock::now().time_since_epoch().count());
        msequence++;
    }
}

void FileSink::make_file_name()
{
    if (!make_directory(mlogDirectory)) {
        fprintf(stderr, "Slog: Could not create log directory %s\n", mlogDirectory);
    }
    snprintf(mfullLogName, sizeof(mfullLogName), "%s/%s_%s_%03d.%s", mlogDirectory, mlogBaseName, msessionStartTimeStr, msequence,
             mlogExtension);
}

void FileSink::record(LogRecord const& rec)
{    
    open_or_rotate();
    if (nullptr == mfile) { return; }
    mbytesWritten += mformat(mfile, rec);
    fputc('\n', mfile);
    mbytesWritten++;
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
