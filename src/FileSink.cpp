#include "FileSink.hpp"
#include <cstring>
#include <cerrno>
#include <limits>
#include "SinkTools.hpp"
#include <chrono>
#include <ctime>
namespace slog {

FileSink::FileSink() {
    mfile = nullptr;
    time_t t = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tm broken;
    gmtime_r(&t, &broken);
    strftime(msessionStartTime, sizeof(msessionStartTime), "%Y%m%dT%H%M%SZ", &broken);
    set_max_file_size(std::numeric_limits<int>::max() - 2048);
    set_echo(true);
    set_file(".", program_invocation_short_name);
    set_formatter(default_format);
}

void FileSink::set_max_file_size(int isize) {
    mmaxBytes = isize;
    msequence = 0;
    mbytesWritten = 0;
}


void FileSink::set_file(char const* location, char const* name, char const* end) {
    strncpy(mfileLocation, location, sizeof(mfileLocation));
    strncpy(mfileName, name, sizeof(mfileName));
    strncpy(mfileEnd, end, sizeof(mfileEnd));
    if (mfile) {
        fclose(mfile);
    }
    mfile = nullptr;
}

void FileSink::open_or_rotate() {
    if (mbytesWritten > mmaxBytes) {
        if (mfile) {
            fclose(mfile);
        }
        mfile = nullptr;
        mbytesWritten = 0;
    }

    if (mfile == nullptr) {
        char fname[sizeof(mfileLocation) + sizeof(mfileName) + sizeof(mfileEnd)
                                         + sizeof(msessionStartTime) + 8];
        snprintf(fname, sizeof(fname), "%s/%s_%s_%03d.%s",
                 mfileLocation, mfileName, msessionStartTime, msequence, mfileEnd);
        msequence++;
        mfile = fopen(fname, "w");
    }
}

void FileSink::record(const LogRecord* rec) {
    open_or_rotate();
    mbytesWritten += mformat(mfile, rec);
    fprintf(mfile, "\n");
    fflush(mfile);
    if (mecho) {
        mformat(stdout, rec);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
}

}
