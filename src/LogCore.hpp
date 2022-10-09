#pragma once
#include "LogRecord.hpp"

namespace slog {
namespace detail
{
bool should_log(int level, char const* tag="", int channel=DEFAULT_CHANNEL);

void take_record(LogRecord* rec);

LogRecord* allocate_record(int channel=DEFAULT_CHANNEL);

}
}

