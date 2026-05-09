#include "LogSink.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "ConsoleSink.hpp"
#include "SlogConfig.hpp"
#include "PlatformUtilities.hpp"
#include "slog/Timestamp.hpp"

namespace slog
{

char const* severity_string(int severity)
{
    if (severity >= DBUG) {
        return "DBUG";
    } else if (severity >= INFO) {
        return "INFO";
    } else if (severity >= NOTE) {
        return "NOTE";
    } else if (severity >= WARN) {
        return "WARN";
    } else if (severity >= ERRR) {
        return "ERRR";
    } else if (severity >= CRIT) {
        return "CRIT";
    } else if (severity >= ALRT) {
        return "ALRT";
    } else if (severity >= EMER) {
        return "EMER";
    } else {
        return "FATL";
    }
}

void format_severity(char* severity_str, int severity)
{
    char const* severity_const_string = severity_string(severity);
    strncpy(severity_str, severity_const_string, 5);
}

void format_location(char* location_str, char const* file_name, int line_number)
{
    snprintf(location_str, 63, "%s@%d", basename(file_name), line_number);
}


uint32_t total_record_size(LogRecord const& rec)
{
    uint32_t total_size = 0;
    LogRecord const* r = &rec;
    do {
        total_size += static_cast<uint32_t>(r->size());
        r = r->more();
    } while (r);
    return total_size;
}

long write_message_to_file(FILE* sink, LogRecord const& rec)
{
    long count = 0;
    count += fwrite(rec.message(), sizeof(char), rec.size(), sink);
    for (LogRecord const* more = rec.more(); more != nullptr; more = more->more()) {
        count += fwrite(more->message(), sizeof(char), more->size(), sink);
    }
    return count;
}


long default_format(FILE* sink, LogRecord const& rec)
{
    char time_str[32];
    rec.meta().time().format_time(time_str, 3, Timestamp::FULL_T);    
    long write_count = 0;
    fputc('[', sink);
    write_count++;
    write_count += fwrite(severity_string(rec.meta().severity()), sizeof(char), 4, sink);
    fputc(' ', sink);
    write_count++;
    if (rec.meta().tag()[0]) {
        write_count += fwrite(rec.meta().tag(), sizeof(char), strnlen(rec.meta().tag(), TAG_SIZE), sink);
        fputc(' ', sink);
        write_count++;
    }
    write_count += fwrite(time_str, sizeof(char), strnlen(time_str, sizeof(time_str)), sink);
    write_count += fwrite("] ", sizeof(char), 2, sink);
    write_count += write_message_to_file(sink, rec);
    return write_count;
}

long no_meta_format(FILE* sink, LogRecord const& rec)
{
    return write_message_to_file(sink, rec);
}


long long_binary_format(FILE* sink, LogRecord const& rec)
{
    // Write the record leader
    long total_bytes = 0;
    uint32_t record_size = static_cast<uint32_t>(total_record_size(rec));
    int32_t reduced_severity = static_cast<int32_t>(rec.meta().severity());
    total_bytes += fwrite(&record_size, sizeof(uint32_t), 1, sink);
    total_bytes += fwrite(&reduced_severity, sizeof(int32_t), 1, sink);
    auto time = rec.meta().time();
    total_bytes += fwrite(&time, sizeof(uint64_t), 1, sink);
    total_bytes += fwrite(rec.meta().tag(), sizeof(char), TAG_SIZE, sink);

    // Write the message
    total_bytes += write_message_to_file(sink, rec);    
    return total_bytes;
}

long default_binary_format(FILE* sink, LogRecord const& rec)
{
    // Write the record leader
    long total_bytes = 0;
    uint32_t record_size = total_record_size(rec);
    total_bytes += fwrite(&record_size, sizeof(uint32_t), 1, sink);
    total_bytes += fwrite(rec.meta().tag(), sizeof(char), 4, sink);

    // Write the message
    total_bytes += write_message_to_file(sink, rec);    
    return total_bytes;
}

long default_binary_header_furniture(FILE* sink, int sequence, uint64_t /*time*/)
{
    char const* MAGIC = "SLOG";
    uint16_t const BOM = 0xfeff;
    uint16_t const short_sequence = static_cast<uint16_t>(sequence);
    long count = 0;
    count += fwrite(MAGIC, sizeof(char), 4, sink);
    count += fwrite(&BOM, sizeof(uint16_t), 1, sink);
    count += fwrite(&short_sequence, sizeof(uint16_t), 1, sink);
    return count;
}

} // namespace slog
