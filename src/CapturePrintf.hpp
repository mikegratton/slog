#pragma once
#include "LogRecord.hpp"

namespace slog 
{
LogRecord* capture_message(LogRecord* rec, char const* format, ...);
}
