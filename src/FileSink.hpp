#pragma once
#include "LogSink.hpp"
#include <cstdio>

namespace slog {

class FileSink : public LogSink {
public:
    FileSink();
    ~FileSink() { if (mfile) { fclose(mfile); } }

    virtual void record(LogRecord const& rec) override;

    void set_formatter(Formatter format) { mformat = format; }

    void set_echo(bool doit=true) { mecho = doit; }

    void set_max_file_size(int isize);

    void set_file(char const* location, char const* name, char const* end="log");

protected:

    void open_or_rotate();

    FILE* mfile;
    Formatter mformat;
    char mfileLocation[256];
    char mfileName[128];
    char mfileEnd[128];
    char msessionStartTime[20];
    int msequence;
    int mbytesWritten;
    int mmaxBytes;
    bool mecho;
};

}
