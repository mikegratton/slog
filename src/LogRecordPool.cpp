#include "LogRecordPool.hpp"
#include <cassert>
#include <cstdlib>
#include <new>
#include <vector>

namespace slog {



class PoolMemory {
public:

    ~PoolMemory() {
        for (char* ptr : allocations) {
            if (ptr) {
                free(ptr);
            }
        }
    }

    char* allocate(uint64_t size) {
        allocations.push_back((char*) malloc(size));
        return allocations.back();
    }

private:
    std::vector<char*> allocations;
};


void LogRecordPool::allocate() {
    long record_size = sizeof(RecordNode);
    long message_size = m_chunkSize - record_size;
    char* pool = m_pool->allocate(m_chunkSize*m_chunks);
    if (nullptr == pool) { // Memory exhausted
        return;
    }
    RecordNode* here = nullptr;
    RecordNode* next = nullptr;
    // Link nodes, invoking the LogRecord constructor via placement new
    for (long i=m_chunks-1; i>=0; i--) {
        here = reinterpret_cast<RecordNode*>(pool + i*m_chunkSize);
        char* message = pool + i*m_chunkSize + record_size;
        new (&here->rec) LogRecord(message, message_size);
        here->next = next;
        here->jumbo = nullptr;
        next = here;
    }

    if (m_cursor) {
        // put existing records at end of new stack
        RecordNode* bottom = reinterpret_cast<RecordNode*>(pool + (m_chunks-1)*m_chunkSize);
        bottom->next = m_cursor;
    }
    m_cursor = here;
}

LogRecordPool::LogRecordPool(LogRecordPoolPolicy i_policy, long i_alloc_size,
        long i_message_size, long i_max_blocking_time_ms) {
    m_policy = i_policy;
    m_max_blocking_time_ms = i_max_blocking_time_ms;
    m_pool = new PoolMemory;
    m_cursor = nullptr;
    long record_size = sizeof(RecordNode);
    m_chunkSize = record_size + i_message_size;
    m_chunks = i_alloc_size / m_chunkSize;
    if (m_chunks < 1) {
        m_chunks = 1;
    }

    allocate();
}

LogRecordPool::~LogRecordPool() { delete m_pool; }

RecordNode* LogRecordPool::take() {    
    RecordPtr allocated = nullptr;
    switch (m_policy) {
    case ALLOCATE: {
        std::unique_lock<std::mutex> guard(m_lock);
        if (nullptr == m_cursor) {
            allocate();
            assert(m_cursor);
        }
        allocated = m_cursor;
        m_cursor = m_cursor->next;
        return allocated;
    }
    case BLOCK: {
        std::unique_lock<std::mutex> guard(m_lock);
        std::chrono::milliseconds wait{m_max_blocking_time_ms};
        if (m_nonempty.wait_for(guard, wait, [this]() -> bool {return m_cursor != nullptr;})) {
            allocated = m_cursor;
            m_cursor = m_cursor->next;
        }
        return allocated;
    }
    case DISCARD: 
    default : {
        std::unique_lock<std::mutex> guard(m_lock);
        allocated = m_cursor;
        if (m_cursor) {
            m_cursor = m_cursor->next;
        }
        return allocated;
    }
    }    
}

void LogRecordPool::put(RecordNode* node) {
    if (node) {
        if (node->jumbo) {
            put(node->jumbo);
        }
        node->jumbo = nullptr;
        node->rec.reset();
        std::unique_lock<std::mutex> guard(m_lock);
        node->next = m_cursor;
        m_cursor = node;
        guard.unlock();
        if (m_policy == BLOCK) {
            m_nonempty.notify_one();
        }
    }
}

long LogRecordPool::count() const {
    std::unique_lock<std::mutex> guard(m_lock);
    long c = 0;
    RecordPtr cursor = m_cursor;
    while (cursor) {
        c++;
        cursor = cursor->next;
    }
    return c - m_chunks;
}
}
