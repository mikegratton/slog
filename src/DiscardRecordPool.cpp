#include "LogConfig.hpp"
#include "DiscardRecordPool.hpp"
#include <cassert>
#include <cstdlib>
#include <new>

namespace slog {
    
constexpr std::chrono::milliseconds WAIT{50};

DiscardRecordPool::DiscardRecordPool(long max_size, long message_size) {
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

DiscardRecordPool::~DiscardRecordPool() {
    free(mpool);
}

RecordNode* DiscardRecordPool::take() {    
    RecordPtr allocated = nullptr;
    std::unique_lock<std::mutex> guard(mlock);    
    allocated = mcursor;
    if (mcursor) {
        mcursor = mcursor->next; 
    }    
    return allocated;
}

void DiscardRecordPool::put(RecordNode* node) {
    if (node) {
        node->rec.reset();
        std::unique_lock<std::mutex> guard(mlock);
        node->next = mcursor;
        mcursor = node;
    }
}

long DiscardRecordPool::count() const {
    std::unique_lock<std::mutex> guard(mlock);
    long c = 0;
    RecordPtr cursor = mcursor;
    while (cursor) {
        c++;
        cursor = cursor->next;
    }
    return c - mchunks;
}
}
