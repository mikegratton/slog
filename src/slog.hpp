#pragma once
#include "CaptureStream.hpp"

#define SLOG_Logs(level) SLOG_LogStreamBase(level, "", slog::DEFAULT_CHANNEL)
#define SLOG_Logst(level, tag) SLOG_LogStreamBase(level, tag, slog::DEFAULT_CHANNEL)
#define SLOG_Logstc(level, tag, channel) SLOG_LogStreamBase(level, tag, channel)

#define SLOG_GET_MACRO(_1,_2,_3,NAME,...) NAME
#define Log(...) SLOG_GET_MACRO(__VA_ARGS__, SLOG_Logstc, SLOG_Logst, SLOG_Logs)(__VA_ARGS__)
