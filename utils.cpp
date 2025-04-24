#include <iostream>
using namespace std;

#include "utils.h"

//=========================================================
// 重载全局operator new/delete

MemoryTracker* MemoryTracker::current = nullptr;

static void* doNew(size_t size, NEW_TYPE newType) {
    // if enabled, use the MemoryTracker to malloc and track memory.
    if (MemoryTracker::getCurrent()) {
        return MemoryTracker::getCurrent()->allocation(size, newType);
    }
    return malloc(size);
}

static void doDelete(void* ptr, NEW_TYPE newType, size_t size = 0) {
    if (MemoryTracker::getCurrent()) {
        MemoryTracker::getCurrent()->deallocation(ptr, newType, size);
        return;
    }
    free(ptr);
}

void* operator new(size_t size) {
    return doNew(size, NEW_SINGLE);
}

// 从 C++14 开始，编译器在某些场景下（尤其是有非平凡析构函数或有初始值时）
// 会调用带 size 的 operator delete(void*, size_t)。这叫作 sized deallocation。
void operator delete(void* ptr, size_t size) noexcept {
    doDelete(ptr, NEW_SINGLE, size);
}

// 重载 operator new[]
void* operator new[](size_t size) {
    return doNew(size, NEW_ARRAY);
}

// 重载 operator delete[], 这个用没有带size的版本
void operator delete[](void* ptr) {
    doDelete(ptr, NEW_ARRAY);
}
