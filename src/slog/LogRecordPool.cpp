#include "LogRecordPool.hpp"

#include <cassert>
#include <cstdlib>
#include <new>
#include <vector>

namespace slog {

NodePtr toNodePtr(LogRecord const* i_rec)
{
    constexpr std::ptrdiff_t OFFSET = sizeof(NodePtr);
    if (i_rec) {
        // This is some ugly pointer math.
        char* rec = reinterpret_cast<char*>(const_cast<LogRecord*>(i_rec));
        return reinterpret_cast<NodePtr>(rec - OFFSET);
    }
    return nullptr;
}

void attach(NodePtr io_rec, NodePtr i_jumbo)
{
    if (io_rec && i_jumbo) { io_rec->rec.more = &i_jumbo->rec; }
}

class PoolMemory {
   public:
    ~PoolMemory()
    {
        for (char* ptr : allocations) {
            if (ptr) { free(ptr); }
        }
    }

    char* allocate(uint64_t size)
    {
        allocations.push_back((char*)malloc(size));
        return allocations.back();
    }

   private:
    std::vector<char*> allocations;
};

void LogRecordPool::acquire_blank_records()
{
    long record_size = sizeof(RecordNode);
    long message_size = m_chunkSize - record_size;
    char* pool = m_pool->allocate(m_chunkSize * m_chunks);
    if (nullptr == pool) {  // Memory exhausted
        return;
    }
    RecordNode* here = nullptr;
    RecordNode* next = nullptr;
    // Link nodes, invoking the LogRecord constructor via placement new
    for (long i = m_chunks - 1; i >= 0; i--) {
        here = reinterpret_cast<RecordNode*>(pool + i * m_chunkSize);
        char* message = pool + i * m_chunkSize + record_size;
        new (&here->rec) LogRecord(message, message_size);
        here->next = next;
        next = here;
    }

    if (m_cursor) {
        // put existing records at end of new stack
        RecordNode* bottom = reinterpret_cast<RecordNode*>(pool + (m_chunks - 1) * m_chunkSize);
        bottom->next = m_cursor;
    }
    m_cursor = here;
}

LogRecordPool::LogRecordPool(LogRecordPoolPolicy i_policy, long i_alloc_size, long i_message_size,
                             long i_max_blocking_time_ms)
{
    m_policy = i_policy;
    m_max_blocking_time_ms = i_max_blocking_time_ms;
    m_pool = new PoolMemory;
    m_cursor = nullptr;
    long record_size = sizeof(RecordNode);
    m_chunkSize = record_size + i_message_size;
    m_chunks = i_alloc_size / m_chunkSize;
    if (m_chunks < 1) { m_chunks = 1; }

    acquire_blank_records();
}

LogRecordPool::~LogRecordPool()
{
    delete m_pool;
}

RecordNode* LogRecordPool::allocate()
{
    NodePtr allocated = nullptr;
    switch (m_policy) {
        case ALLOCATE: {
            std::unique_lock<std::mutex> guard(m_lock);
            if (nullptr == m_cursor) {
                acquire_blank_records();
                assert(m_cursor);
            }
            allocated = m_cursor;
            m_cursor = m_cursor->next;
            break;
        }
        case BLOCK: {
            std::chrono::milliseconds wait{m_max_blocking_time_ms};
            std::unique_lock<std::mutex> guard(m_lock);
            if (m_nonempty.wait_for(guard, wait, [this]() -> bool { return m_cursor != nullptr; })) {
                allocated = m_cursor;
                m_cursor = m_cursor->next;
            }
            break;
        }
        case DISCARD:
        default: {
            std::unique_lock<std::mutex> guard(m_lock);
            allocated = m_cursor;
            if (m_cursor) { m_cursor = m_cursor->next; }
            break;
        }
    }
    return allocated;
}

void LogRecordPool::free(RecordNode* node)
{
    if (node) {
        if (node->rec.more) { free(toNodePtr(node->rec.more)); }
        node->rec.reset();
        std::unique_lock<std::mutex> guard(m_lock);
        node->next = m_cursor;
        m_cursor = node;
        guard.unlock();
        if (m_policy == BLOCK) { m_nonempty.notify_one(); }
    }
}

long LogRecordPool::count() const
{
    std::unique_lock<std::mutex> guard(m_lock);
    long c = 0;
    NodePtr cursor = m_cursor;
    while (cursor) {
        c++;
        cursor = cursor->next;
    }
    return c;
}
}  // namespace slog
