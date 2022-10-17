#include "LogConfig.hpp"
#if SLOG_LOCK_FREE_POOL
#include "LfRecordPool.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>

namespace slog {
    
RecordPtr& RecordPtr::operator=(RecordNode* ptr) {
    memcpy(this, &ptr, sizeof(ptr));
    return *this;
}

RecordNode* RecordPtr::get(char* origin) {
    // Using an offset so all zeros is a nullptr
    uint64_t raw = reinterpret_cast<uint64_t>(origin) + moffset - 1;
    return reinterpret_cast<RecordNode*>(raw);
}

RecordPtr& RecordPtr::set(RecordNode* node, char* origin, int tag) {
    if (nullptr == node) {
        moffset = 0;
        mtag = tag;
        return *this;
    }

    assert((char*) node >= origin);
    // Using an offset so all zeros is a nullptr
    moffset = static_cast<uint32_t>(reinterpret_cast<char*>(node) - origin) + 1;
    mtag = tag;
    return *this;
}

bool RecordPtr::operator==(RecordNode const* ptr) const 
{ 
    return 0 == memcmp(this, &ptr, sizeof(RecordPtr)); 
    
}


LfLogRecordPool::LfLogRecordPool(long max_size, long message_size)
    : mcursor(RecordPtr()) 
    {
        
    assert(sizeof(RecordPtr) == 8);
    
    long record_size = sizeof(RecordNode);
    mchunkSize = record_size + message_size;
    mchunks = max_size / mchunkSize;
    if (mchunks < 1) {
        mchunks = 1;
    }
    mpool = (char*) malloc(mchunkSize*mchunks);
    RecordNode* here = nullptr;
    RecordNode* next = nullptr;
    // Link nodes, invoking the LogRecord constructor via placement new
    for (long i=mchunks-1; i>=0; i--) {
        here = reinterpret_cast<RecordNode*>(mpool + i*mchunkSize);
        char* message = mpool + i*mchunkSize + record_size;
        new (&here->rec) LogRecord(message, message_size);
        here->next.set(next, mpool, 0);
        next = here;
    }
    assert((char*)here == mpool);
    mcursor.store(RecordPtr(here, mpool, 0));
}


LfLogRecordPool::~LfLogRecordPool() {
    free(mpool);
}

RecordNode* LfLogRecordPool::take() {
    // Pop
    RecordPtr record = mcursor.load(std::memory_order_relaxed);
    while (record && !mcursor.compare_exchange_weak(record, record.get(mpool)->next,
                                                    std::memory_order_acquire,
                                                    std::memory_order_relaxed)) {
        ; // Empty
    }
    return record.get(mpool);
}

void LfLogRecordPool::put(RecordNode* record) {
    // Push
    if (record) {
        record->rec.reset();
        RecordPtr recordp(record, mpool, 0 + 1); // FIXME
        record->next = mcursor.load(std::memory_order_relaxed);
        while (!mcursor.compare_exchange_weak(record->next, recordp,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed)) {
            ; // Empty
        }
    }
}

long LfLogRecordPool::count() const {
    long c = 0;
    RecordPtr cursor = mcursor;
    while (cursor) {
        c++;
        cursor = cursor.get(mpool)->next;
    }
    return c - mchunks;
}

}
#endif
