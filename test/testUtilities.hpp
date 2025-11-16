#pragma once
#include "slog/LogRecord.hpp"
#include <string>

namespace slog
{

struct TestLogRecord {
    LogRecordMetadata meta;
    long message_byte_count;
    char message[2048];
};

void rmdir(char const* dirname);

void parse_log_record(TestLogRecord& o_record, char const* recordString);

std::string get_path_to_test();

FILE* popen2(char const* command, char const* mode, pid_t* pid);

int wait_for_child(int pid);

} // namespace slog