#include <iostream>
using namespace std;

#include "utils.h"

constexpr uint32_t tailMagic = 0xDEADBEEF;
constexpr int tailSize = sizeof(uint32_t);  // 4 byte

thread_local MemoryTracker::ThreadStats MemoryTracker::tls_stats{};
MemoryTracker::GlobalStats MemoryTracker::global_stats{};
bool MemoryTracker::trackingEnabled = false;

void* MemoryTracker::allocate(size_t userSize, NEW_TYPE newType) {
    assert(newType == NEW_SINGLE || newType == NEW_ARRAY);  // internal error
    if (!trackingEnabled) {
        return malloc(userSize);
    }

    size_t totalSize = sizeof(Header) + userSize + tailSize;
    void* base = malloc(totalSize);
    if (!base) {
        return nullptr;
    }

    if (debug) {
        PRINTF("allocate, userSize = %zu, newType = %d, base = %p\n", userSize, newType, base);
    }

    // fill the header.
    Header* h = reinterpret_cast<Header*>(base);
    h->base = base;
    h->totalSize = totalSize;
    h->newType = newType;

    // fill the tail
    assert(tailSize == sizeof(uint32_t));  // internal error
    uint32_t* tail = reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(base) + totalSize - tailSize);
    *tail = tailMagic;

    tls_stats.insertToDDL(h);
    tls_stats.localNewCnt++;
    tls_stats.localNewMemSize += totalSize;

    void* userPtr = reinterpret_cast<void*>(h + 1);
    return userPtr;
}

void MemoryTracker::deallocate(void* userPtr, NEW_TYPE newType, size_t userSize) {
    assert(newType == NEW_SINGLE || newType == NEW_ARRAY);  // internal error
    if (!userPtr) {
        assert(false);  // internal error
        return;
    }
    if (!trackingEnabled) {
        return free(userPtr);
    }

    Header* h = reinterpret_cast<Header*>(userPtr) - 1;

    void* base = reinterpret_cast<void*>(h);
    // ASSERT_E(h->base == ptr && "Error: the base of the total memory is not correct.", MEM_OVERRUN);
    assert(h->base == base && "Error: the base of the total memory is not correct.");

    size_t expectedUserSize = h->totalSize - sizeof(Header) - tailSize;
    // cout << "userSize = " << userSize << ", expectedUserSize = " << expectedUserSize << endl;
    assert((userSize == 0 || userSize == expectedUserSize) && "Error: user memory size is not correct.");

    // ASSERT_E(h->newType == newType && "new and delete mismatch.", NEW_DELETE_MISMATCH);
    assert(h->newType == newType && "new and delete mismatch.");

    // check the tail.
    uint32_t* tail = reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(base) + h->totalSize - tailSize);
    assert(*tail == tailMagic && "memory overrun detected!");

    if (debug) {
        PRINTF("deallocate, userSize = %zu, newType = %d, base = %p\n", (size_t)0UL, newType, base);
    }

    // record the info before freeing.
    tls_stats.localDeleteCnt++;
    tls_stats.localDeleteMemSize += h->totalSize;
    tls_stats.removeFromDDL(h);

    free(base);
}
