#pragma once

#if SLOG_POOL_POLICY == SLOG_EMPTY_POOL_BLOCKS
#include "BlockingRecordPool.hpp"
namespace slog 
{
using LogRecordPool = BlockingRecordPool;
}
#elif SLOG_POOL_POLICY == SLOG_EMPTY_POOL_DISCARDS
#include "DiscardRecordPool.hpp"
namespace slog 
{
using LogRecordPool = DiscardRecordPool;
}
#elif SLOG_POOL_POLICY == SLOG_EMPTY_POOL_ALLOCATES
#include "AllocatingRecordPool.hpp"
namespace slog 
{
using LogRecordPool = AllocatingRecordPool;
}
#endif
