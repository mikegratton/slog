#include "LogRecordPool.hpp"

namespace slog {

LogRecordPool::LogRecordPool(long max_size, long message_size) {
    long record_size = sizeof(LogRecord);
    mmessageSize = message_size;
    mchunkSize = record_size + message_size;
    mchunks = max_size / mchunkSize;
    if (mchunks < 1) {
        mchunks = 1;
    }
    char* rawpool = (char*) malloc(mchunkSize*mchunks);
    mpool = reinterpret_cast<LogRecord*>(rawpool);
    LogRecord* prev = nullptr;    
    // Link chunks
    for (long i=mchunks-1; i>=0; i--) {
        LogRecord* here = reinterpret_cast<LogRecord*>(rawpool + i*mchunkSize);
        new(here) LogRecord(mmessageSize);                
        here->next = prev;        
        prev = here;
    }
    mcursor = mpool;
}

LogRecordPool::~LogRecordPool() {
    free(mpool);
}

LogRecord* LogRecordPool::allocate() {
    LogRecord* allocated = nullptr;

    {
        std::lock_guard<std::mutex> guard(mlock);
        allocated = mcursor;
        if (allocated) {
            mcursor = mcursor->next;
        }
    }

    if (allocated) {
        allocated->reset();
    }
    return allocated;
}

void LogRecordPool::deallocate(LogRecord* record) {
    if (record) {
        std::lock_guard<std::mutex> guard(mlock);
        record->next = mcursor;
        mcursor = record;
    }
}

}
