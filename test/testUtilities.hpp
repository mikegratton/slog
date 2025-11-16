#pragma once
#include "slog/LogRecord.hpp"
#include "slog/PlatformUtilities.hpp"

namespace slog
{

struct TestLogRecord {    
    LogRecordMetadata meta;
    long message_byte_count;
    char message[2048];
};

void rmdir(char const* dirname);

void parseLogRecord(TestLogRecord& o_record, char const* recordString);

}