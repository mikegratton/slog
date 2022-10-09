#pragma once
#include "LogCore.hpp"

#define Logf(level, ...) SLOG_LogBase(level, "", slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Logft(level, tag, ...) SLOG_LogBase(level, tag, slog::DEFAULT_CHANNEL, __VA_ARGS__)
#define Logftc(level, tag, channel, ...) SLOG_LogBase(level, tag, channel, __VA_ARGS__)
