#include "LogConfig.hpp"
#if SLOG_LOCK_FREE_POOL
#include "LfRecordPool.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
#include <algorithm>

namespace slog {


namespace {
    union Splitter {
        RecordPtr ptr;
        uint32_t split[2]; // [0] one-based index of record; 
                           // [1] tag for ABA prevention
        uint32_t& index() { return split[0]; }
        uint32_t& tag() { return split[1]; }
    };
}

    
LfLogRecordPool::LfLogRecordPool(long max_size, long message_size)
    : mcursor(RecordPtr()) {

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
        here->next = to_packed(next);
        next = here;
    }
    assert((char*)here == mpool);
    mcursor.store(to_packed(here));
    
}


LfLogRecordPool::~LfLogRecordPool() {
    free(mpool);
}

RecordPtr LfLogRecordPool::take() {

    RecordPtr popped_packed = mcursor.load(std::memory_order_relaxed);
 
    while(!(is_packed_nullptr(popped_packed) ||
          mcursor.compare_exchange_weak(popped_packed, packed_next(popped_packed)))) {
        increment_packed(popped_packed);
    }
    return to_full_ptr(popped_packed);    
}

void LfLogRecordPool::put(RecordPtr record) {

    if (nullptr == record) {
        return;
    }
    RecordPtr pushed_packed = to_packed(record);
    record->next = mcursor.load(std::memory_order_relaxed);
        
    while(!mcursor.compare_exchange_weak(record->next, pushed_packed)) {        
        increment_packed(record->next);
    }
}


long LfLogRecordPool::count() const {
    long c = 0;
    RecordPtr cursor = mcursor.load();
    while (cursor) {
        c++;
        cursor = packed_next(cursor);
    }
    return c - mchunks;
}

RecordPtr LfLogRecordPool::to_full_ptr(RecordPtr packed) const
{    
    Splitter s{packed};
    if (s.index() == 0) {
        return nullptr;
    }
    /*
     * Undo one-offset
     */
    uint64_t raw = mchunkSize * (s.index()-1) + reinterpret_cast<uint64_t>(mpool);
    return reinterpret_cast<RecordPtr>(raw);
    
}

RecordPtr LfLogRecordPool::to_packed(RecordPtr full, uint32_t tag) const
{
    if (nullptr == full) {
        return nullptr;
    }
    /*
     * Note offset by one so 0 represents a nullptr
     */
    uint64_t index = (reinterpret_cast<uint64_t>(full) 
                        - reinterpret_cast<uint64_t>(mpool))/mchunkSize + 1;
    Splitter s;    
    s.index() = static_cast<uint32_t>(index);
    s.tag() = tag;
    return s.ptr;
}

bool LfLogRecordPool::is_packed_nullptr(RecordPtr const& packed) const
{
    Splitter s{packed};
    return (0 == s.index()); 
}

RecordPtr& LfLogRecordPool::packed_next(RecordPtr& packed) const
{
    if (is_packed_nullptr(packed)) {
        return packed;
    }
    return to_full_ptr(packed)->next;
}

void LfLogRecordPool::increment_packed(RecordPtr& packed) const
{
    Splitter s{packed};
    s.tag()++;
}

}
#endif
