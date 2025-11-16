#include "JournaldSink.hpp"
#include "LogRecord.hpp"
#include "SlogError.hpp"
#include <algorithm>
#include <cstring>
#define SD_JOURNAL_SUPPRESS_LOCATION
#include <string>
#include <systemd/sd-journal.h>

namespace slog
{

JournaldSink::JournaldSink()
    : buffer_size(line_max() - 8),
      formatter(default_format),
      echo(false)
{
    // Reserve one extra byte for the null terminator
    message_buffer = new char[buffer_size + 1];
}

JournaldSink::~JournaldSink() { delete[] message_buffer; }

void JournaldSink::write_to_journal(LogRecord const& rec) const
{
    // We reserve one extra byte for the null
    message_buffer[buffer_used] = '\0';
    // clang-format off
    sd_journal_send(
        "CODE_FUNC=%s",   rec.meta().function(),
        "CODE_FILE=%s",   rec.meta().filename(),
        "CODE_LINE=%d",   rec.meta().line(),
        "THREAD=%ld",     rec.meta().thread_id(),
        "TIMESTAMP=%s",   misotime,
        "PRIORITY=%d",    rec.meta().severity()/100,
        "TAG=%s",         rec.meta().tag(),  
        "MESSAGE=%s",     message_buffer,
        NULL);
    // clang-format on   
}

void JournaldSink::record(LogRecord const& rec) 
{
    format_time(misotime, rec.meta().time());
    if (echo) {
        formatter(stdout, rec);
        fputc('\n', stdout);
        fflush(stdout);
    }

    // Fill the buffer up with the message record. In general, one of our
    // messages should fit in a modern journald line (48K), but we don't make
    // any assumptions. If we have too many chars for the journal, submit for
    // each full buffer, plus one for the remainer.
    buffer_used = 0; // Bytes of buffer filled
    long rec_bytes_used = 0; // Bytes of current record consumed
    LogRecord const* cursor = &rec; // Current record
    do {        
        long write_size = std::min(buffer_size - buffer_used, cursor->size() - rec_bytes_used);
        memcpy(message_buffer + buffer_used, cursor->message() + rec_bytes_used, write_size);
        buffer_used += write_size;        
        rec_bytes_used += write_size;
        if (buffer_used == buffer_size) {
            write_to_journal(rec);
            buffer_used = 0;
        }
        if (rec_bytes_used == cursor->size()) {
            cursor = cursor->more();
            rec_bytes_used = 0;
        }
    } while (cursor);
    if (buffer_used > 0) {
        write_to_journal(rec);
        buffer_used = 0;
    }    
}

long JournaldSink::line_max()
{
    int constexpr LINEMAX_VERSION = 235;
    long constexpr OLD_LINE_MAX = 2048;
    long constexpr DEFAULT_LINE_MAX = 48 * 1024;
    char line[2048];
    long line_max_value = OLD_LINE_MAX;

    // Check if the version is >= LINEMAX_VERSION
    FILE* version_pipe = popen("systemd --version", "r");
    if (!version_pipe) {
        slog_error("Could not determine systemd version. Using to LINE_MAX = %ld\n", line_max_value);
        return line_max_value;
    }
    char identifier[1024];
    int version = -1;
    while (nullptr != fgets(line, sizeof(line)-1, version_pipe)) {
        if (2 == sscanf(line, "%s %d", identifier, &version) && strncmp(identifier, "systemd", 7) == 0) {            
            if (version < LINEMAX_VERSION) {
                pclose(version_pipe);
                return line_max_value;
            }
            break;
        }        
    }
    pclose(version_pipe);
    if (version < LINEMAX_VERSION) {
        // This is true if we never found the version
        slog_error("Could not determine systemd version. Using to LINE_MAX = %ld\n", line_max_value);
        return line_max_value;
    }

    line_max_value = DEFAULT_LINE_MAX;
    FILE* config_pipe = popen("systemd-analyze cat-config systemd/journald.conf", "r");
    if (config_pipe) {        
        // Read line-by-line looking for "LineMax=<value><magnitude>"
        while(nullptr != fgets(line, sizeof(line)-1, config_pipe)) {
            std::string sline(line);
            auto our_key = sline.find("LineMax");
            if (our_key != std::string::npos) {                
                auto equal = sline.find('=');
                auto hash = sline.find('#');
                if (equal != std::string::npos && equal < hash) {
                    std::size_t end_number;
                    long raw_value = stoll(sline.substr(equal+1), &end_number);
                    if (end_number < sline.size()) {
                        switch(sline[end_number]) {
                            case 'K':
                                raw_value *= 1024;
                                break;
                            case 'M':
                                raw_value *= 1024 * 1024;
                                break;
                            case 'G':
                                raw_value *= 1024 * 1024 * 1024;
                                break;
                            case 'T':
                                raw_value *= 1024L * 1024L * 1024L * 1024L;
                                break;
                            default:
                                break;
                        }
                    }
                    line_max_value = raw_value;
                    break;
                }
            }
        }
        pclose(config_pipe);
    }    
    return std::max(line_max_value, 79L);
}

} // namespace slog
