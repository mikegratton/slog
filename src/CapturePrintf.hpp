#pragma once
#include "LogRecord.hpp"

namespace slog 
{
RecordNode* capture_message(RecordNode* rec, char const* format, ...);
}
