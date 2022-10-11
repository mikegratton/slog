#include "LogRecordPool.hpp"

namespace slog {

#ifndef SLOG_LOCK_FREE

MutexLogRecordPool::MutexLogRecordPool(long max_size, long message_size) {
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
        new (here) LogRecord(mmessageSize);
        here->next = prev;
        prev = here;
    }
    mcursor = mpool;
}

MutexLogRecordPool::~MutexLogRecordPool() {
    free(mpool);
}

LogRecord* MutexLogRecordPool::allocate() {
    LogRecord* allocated;

    std::lock_guard<std::mutex> guard(mlock);
    allocated = mcursor;
    if (mcursor) {
        mcursor = mcursor->next;
    }

    return allocated;
}

void MutexLogRecordPool::deallocate(LogRecord* record) {
    if (record) {
        record->reset();
        std::lock_guard<std::mutex> guard(mlock);
        record->next = mcursor;
        mcursor = record;
    }
}

#else

LfLogRecordPool::LfLogRecordPool(long max_size, long message_size) {
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
    // Link chunks & give them their space for the record
    for (long i=mchunks-1; i>=0; i--) {
        LogRecord* here = reinterpret_cast<LogRecord*>(rawpool + i*mchunkSize);
        new (here) LogRecord(mmessageSize);
        here->reset();
        here->next = prev;
        prev = here;
    }
    mcursor = mpool;
}


LfLogRecordPool::~LfLogRecordPool() {
    free(mpool);
}

LogRecord* LfLogRecordPool::allocate() {
    // Pop
    LogRecord* record = mcursor.load(std::memory_order_relaxed);
    while (record && !mcursor.compare_exchange_weak(record, record->next,
            std::memory_order_acquire,
            std::memory_order_relaxed)) {
        ; // Empty
    }
    if (record) {
        record->reset();
    }
    return record;
}

void LfLogRecordPool::deallocate(LogRecord* record) {
    // Push
    if (record) {
        record->reset();
        record->next = mcursor.load(std::memory_order_relaxed);
        while (!mcursor.compare_exchange_weak(record->next, record,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed)) {
            ; // Empty
        }
    }
}
#endif
}
