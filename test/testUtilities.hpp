#pragma once
#include "slog/LogRecord.hpp"
#include "slog/PlatformUtilities.hpp"

namespace slog
{

struct TestLogRecord {    
    LogRecordMetadata meta;    
    long message_byte_count;
    char const* message;
};

void rmdir(char const* dirname);

void parseLogRecord(TestLogRecord& o_record, char const* recordString);

}