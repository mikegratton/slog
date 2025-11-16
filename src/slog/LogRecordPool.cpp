#include "LogRecordPool.hpp"
#include "slog/LogRecord.hpp"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace slog
{

/**
 * This holds all allocations from the heap that are in use by the
 * LogRecordPool. Each request for more memory is served by two allocations: one
 * for the records and one for the message storage in the record.
 *
 * For BLOCK or DISCARD pools, there will only ever be one allocation. For
 * ALLOCATE pools, additional allocations can occur when the LogRecordPool is
 * exhausted.
 */
class PoolMemory
{
  public:
    ~PoolMemory()
    {
        for (auto& item : allocations) {
            delete[] item.first;
            delete[] item.second;
        }
    }

    std::pair<LogRecord*, char*>& allocate(uint64_t count, uint64_t message_size)
    {
        allocations.emplace_back(new LogRecord[count], new char[message_size * count]);
        return allocations.back();
    }

  private:
    std::vector<std::pair<LogRecord*, char*>> allocations;
};

void LogRecordPool::acquire_blank_records()
{
    if (chunks == 0) { return; }
    
    auto& allocation = pool->allocate(chunks, message_size);
    if (nullptr == allocation.first || nullptr == allocation.second) { // Memory exhausted
        return;
    }
    LogRecord* here = nullptr;
    LogRecord* next = head;
    // Link nodes and insert message memory
    for (long i = chunks - 1; i >= 0; i--) {
        here = &allocation.first[i];
        here->m_message = allocation.second + i * message_size;
        here->m_message_max_size = message_size;
        here->m_next = next;
        next = here;
    }
    head = here;
}

LogRecordPool::LogRecordPool(LogRecordPoolPolicy new_policy, long new_alloc_size, long new_message_size,
                             long new_max_blocking_time_ms)
    : policy(new_policy),
      max_blocking_time_ms(new_max_blocking_time_ms),
      message_size(std::max<long>(sizeof(LogRecord), new_message_size)),
      chunks(std::max<long>(16L, new_alloc_size / (sizeof(LogRecord) + message_size))),
      head(nullptr),
      pool(new PoolMemory)
{
    acquire_blank_records();
}

LogRecordPool::~LogRecordPool() { delete pool; }

LogRecord* LogRecordPool::allocate()
{
    NodePtr allocated = nullptr;
    switch (policy) {
    case ALLOCATE: {
        std::unique_lock<std::mutex> guard(lock);
        if (nullptr == head) {
            acquire_blank_records();
            assert(head);
        }
        allocated = head;
        head = head->m_next;
        break;
    }
    case BLOCK: {
        std::chrono::milliseconds wait{max_blocking_time_ms};
        std::unique_lock<std::mutex> guard(lock);
        if (nonempty.wait_for(guard, wait, [this]() -> bool { return head != nullptr; })) {
            allocated = head;
            head = head->m_next;
        }
        break;
    }
    case DISCARD:
    default: {
        std::unique_lock<std::mutex> guard(lock);
        allocated = head;
        if (head) {
            head = head->m_next;
        }
        break;
    }
    }
    return allocated;
}

void LogRecordPool::free(LogRecord* node)
{
    if (node) {
        if (node->m_more) {
            free(node->m_more);
        }
        node->reset();
        std::unique_lock<std::mutex> guard(lock);
        node->m_next = head;
        head = node;
        guard.unlock();
        if (policy == BLOCK) {
            nonempty.notify_one();
        }
    }
}

long LogRecordPool::count() const
{
    std::unique_lock<std::mutex> guard(lock);
    long c = 0;
    NodePtr cursor = head;
    while (cursor) {
        c++;
        cursor = cursor->m_next;
    }
    return c;
}
} // namespace slog
