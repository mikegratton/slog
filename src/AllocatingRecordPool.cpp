#include "LogConfig.hpp"
#include "AllocatingRecordPool.hpp"
#include <cassert>
#include <cstdlib>
#include <new>
#include <vector>

namespace slog {
    
constexpr std::chrono::milliseconds WAIT{50};

class PoolMemory
{
public:
    
    ~PoolMemory() 
    {
        for (char* ptr : allocations) {
            free(ptr);
        }
    }
    
    char* allocate(uint64_t size) {
        allocations.push_back((char*) malloc(size));
        return allocations.back();
    }
    
private:
    std::vector<char*> allocations;
};
void AllocatingRecordPool::allocate()
{
    long record_size = sizeof(RecordNode);
    long message_size = mchunkSize - record_size;
    char* pool = mpool->allocate(mchunkSize*mchunks);
    RecordNode* here = nullptr;
    RecordNode* next = nullptr;
    // Link nodes, invoking the LogRecord constructor via placement new
    for (long i=mchunks-1; i>=0; i--) {
        here = reinterpret_cast<RecordNode*>(pool + i*mchunkSize);
        char* message = pool + i*mchunkSize + record_size;
        new (&here->rec) LogRecord(message, message_size);
        here->next = next;
        next = here;
    }
    assert((char*)here == mpool);
    if (mcursor) {
        // put existing records at end of new stack
        RecordNode* bottom = reinterpret_cast<RecordNode*>(pool + (mchunks-1)*mchunkSize);
        bottom->next = mcursor;                
    }
    mcursor = here;
}

AllocatingRecordPool::AllocatingRecordPool(long max_size, long message_size) {
    mpool = new PoolMemory;
    mcursor = nullptr;    
    long record_size = sizeof(RecordNode);
    mchunkSize = record_size + message_size;
    mchunks = max_size / mchunkSize;
    if (mchunks < 1) {
        mchunks = 1;
    }
    
    allocate();
}

AllocatingRecordPool::~AllocatingRecordPool() { delete mpool; } 

RecordNode* AllocatingRecordPool::take() {    
    std::unique_lock<std::mutex> guard(mlock);
    RecordPtr allocated = mcursor;
    if (mcursor) {
        mcursor = mcursor->next;        
    } else {
        allocate();
        allocated = mcursor;
        assert(allocated);
        mcursor = mcursor->next;        
    }
    return allocated;

}

void AllocatingRecordPool::put(RecordNode* node) {
    if (node) {
        node->rec.reset();
        std::unique_lock<std::mutex> guard(mlock);
        node->next = mcursor;
        mcursor = node;
    }
}

long AllocatingRecordPool::count() const {
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
