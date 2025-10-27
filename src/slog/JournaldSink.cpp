#include "JournaldSink.hpp"
#define SD_JOURNAL_SUPPRESS_LOCATION
#include <systemd/sd-journal.h>

namespace slog {
void JournaldSink::record(LogRecord const& rec) {
    if (mecho) {
        mformat(stdout, rec);
        fputc('\n', stdout);
        fflush(stdout);
    }

    char isoTime[32];
    format_time(isoTime, rec.meta.time);
    sd_journal_send(
        "CODE_FUNC=%s", rec.meta.function,
        "CODE_FILE=%s", rec.meta.filename,
        "CODE_LINE=%d", rec.meta.line,
        "THREAD=%ld",   rec.meta.thread_id,
        "TIMESTAMP=%s", isoTime,
        "PRIORITY=%d",  rec.meta.severity/100,
        "TAG=%s",       rec.meta.tag,  
        "MESSAGE=%s",   rec.message,
        NULL);
    for(LogRecord const* more = rec.more; more != nullptr; more = more->more) {
        sd_journal_send(
            "CODE_FUNC=%s", rec.meta.function,
            "CODE_FILE=%s", rec.meta.filename,
            "CODE_LINE=%d", rec.meta.line,
            "THREAD=%ld",   rec.meta.thread_id,
            "TIMESTAMP=%s", isoTime,
            "PRIORITY=%d",  rec.meta.severity/100,
            "TAG=%s",       rec.meta.tag,
            "MESSAGE=%s",   more->message,
            NULL);
    }
}
}

