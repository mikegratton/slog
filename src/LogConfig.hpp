#pragma once

#ifndef SLOG_MAX_CHANNEL
#define SLOG_LOG_MAX_CHANNEL 4
#endif

#ifndef SLOG_LOGGING_ENABLED
#define SLOG_LOGGING_ENABLED 1
#endif

#ifndef SLOG_LOG_MESSAGE_SIZE
#define SLOG_LOG_MESSAGE_SIZE 1024
#endif

#ifndef SLOG_LOG_POOL_SIZE
#define SLOG_LOG_POOL_SIZE 1048576
#endif

// #define SLOG_DUMP_TO_CONSOLE_WHEN_STOPPED
#define SLOG_LOCK_FREE

namespace slog
{
constexpr int DEFAULT_CHANNEL = 0;
}
