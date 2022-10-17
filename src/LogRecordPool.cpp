#include "LogConfig.hpp"
#if !SLOG_LOCK_FREE_POOL
#include "LogRecordPool.hpp"
#include <cassert>
#include <cstdlib>
#include <new>

namespace slog {

MutexLogRecordPool::MutexLogRecordPool(long max_size, long message_size) {
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
        here->next = next;
        next = here;
    }
    assert((char*)here == mpool);
    mcursor = here;
}

MutexLogRecordPool::~MutexLogRecordPool() {
    free(mpool);
}

RecordNode* MutexLogRecordPool::take() {    
    std::lock_guard<std::mutex> guard(mlock);
    RecordPtr allocated = mcursor;
    if (mcursor) {
        mcursor = mcursor->next;        
    }

    return allocated;
}

void MutexLogRecordPool::put(RecordNode* node) {
    if (node) {
        node->rec.reset();
        std::lock_guard<std::mutex> guard(mlock);
        node->next = mcursor;
        mcursor = node;
    }
}

long MutexLogRecordPool::count() const {
    std::lock_guard<std::mutex> guard(mlock);
    long c = 0;
    RecordPtr cursor = mcursor;
    while (cursor) {
        c++;
        cursor = cursor->next;
    }
    return c - mchunks;
}
}
#endif
