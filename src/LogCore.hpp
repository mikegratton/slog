#pragma once
#include "LogRecord.hpp"

namespace slog {
bool will_log(int severity, char const* tag="", int channel=DEFAULT_CHANNEL);

void push_to_sink(RecordNode* rec);

RecordNode* get_fresh_record(int channel, char const* file, char const* function, int line, 
                           int severity, char const* tag);
}

