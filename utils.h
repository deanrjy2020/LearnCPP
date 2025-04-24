#ifndef UTILS_H
#define UTILS_H

#include <cassert>
#include <sstream>
#include <thread>

//=========================================================
// MemoryTracker结合全局operator new/delete
// 功能:
// - 检测mem leak
// - 检测mem overrun (如果跳过了tail无法检测到)
// - 检测new/delete mismatch, e.g. new + delete[]
//
// - 前面加头, 后面加尾巴, 检测用户拿到/返回的mem是否有异常
// - 用doubly linked list实现
// - 不支持reallocate这种操作
// - 不支持mem size的alignment, 追踪精确的size, 不做.
// - 不支持mem addr的alignment, 这个也会导致size变化, 不做. 单单addr的align见mem_addr_align.cpp
// - 线程不安全
//
// todo, 在DLL实现了已经, 在用hash map实现, 双重检测., 这个在解决下面的问题后再说.
// todo, GPT to review
// todo, memtracker unit test 里面, track test code, 不track std, 用gpt推荐的TLS试试看. 然后再改其他的例子.

enum ERROR_TYPE {
    NO_ERROR,
    MEM_LEAK,
    MEM_OVERRUN,  // out of bounds memory access.
    NEW_DELETE_MISMATCH,
};

enum NEW_TYPE {
    NEW_NONE,  // default value
    NEW_SINGLE,
    NEW_ARRAY,
};

// _E means setting the error type.
#define ASSERT_E(x, e)                                                                                  \
    do {                                                                                                \
        if (enableAssert) {                                                                             \
            assert((x));                                                                                \
        } else if (!(x)) {                                                                              \
            cout << "Assertion failed:\n"                                                               \
                 << "  Expression: " << #x << ", File: " << __FILE__ << ", Line: " << __LINE__ << "\n"; \
            lastError = (e);                                                                            \
            return;                                                                                     \
        }                                                                                               \
    } while (0)

// default ASSERT set the MEM_LEAK type.
#define ASSERT(x) ASSERT_E(x, MEM_LEAK)

const int tailSize = sizeof(uint32_t);  // 4 bytes

// There is only one MT instance at a time, but it is not singleton.
// singleton instance is destroyed at the end of program, but we need
// the MT instance constructed and destroyed for each test.
class MemoryTracker {
private:
    //=========================================================
    // DLL实现
    struct Header {
        Header* prev;  // 8B
        Header* next;
        void* base;
        size_t totalSize;  // = sizeof(Header) + userSize + tailSize
        const char* newFile;
        int newLine;       // 4B
        NEW_TYPE newType;  // should be 4B
                           // no padding
        Header()
            : prev(nullptr), next(nullptr), base(nullptr), totalSize(0), newType(NEW_NONE) {}
    };

    // dummy node of doubly linked list (DLL)
    Header* dummy;

    void insertToDDL(Header*& h) {
        // insert after dummy (dummy's next)
        h->next = dummy->next;
        h->prev = dummy;

        dummy->next = h;
        h->next->prev = h;
    }

    void removeFromDDL(Header*& h) {
        h->next->prev = h->prev;
        h->prev->next = h->next;

        h->next = NULL;
        h->prev = NULL;
    }
    //=========================================================

    size_t allocationCnt;
    size_t deallocationCnt;
    size_t totalAllocatedSize;
    size_t totalDeallocatedSize;
    bool enableAssert;  // 默认开启, 如果关闭, 用log代替
    ERROR_TYPE lastError;

    // one instance for global use.
    static MemoryTracker* current;

public:
    static MemoryTracker*& getCurrent() {
        return current;
    }

    MemoryTracker() {
        init();
    }

    ~MemoryTracker() {
        bool isThisCurrent = (current != nullptr && current == this);
        deinit();

        // cout << "allocationCnt = " << allocationCnt << ", deallocationCnt = " << deallocationCnt
        //      << ", totalAllocatedSize = " << totalAllocatedSize << ", totalDeallocatedSize = " << totalDeallocatedSize
        //      << endl;
        ASSERT(allocationCnt == deallocationCnt && "Error: memory leak.");
        ASSERT(totalAllocatedSize == totalDeallocatedSize && "Error: memory leak");
        // Only printing out memory report when it is current.
        if (isThisCurrent) {
            cout << "[Memory Report] allocationCnt = deallocationCnt = " << allocationCnt
                 << ", totalAllocatedSize = totalDeallocatedSize = " << totalAllocatedSize << " Bytes"
                 << endl;
        }
    }

    void init() {
        // cout << "sizeof(Header) = " << sizeof(Header) << endl;
        assert(sizeof(Header) == 48);  // internal error

        // dummy的这个new是不要track的, 否则有可能自己track自己, 极有可能引入bug.
        assert(current == nullptr);
        // dummy = new Header();
        dummy = (Header*)malloc(sizeof(Header));
        if (!dummy) {
            return;
        }
        // do self loop for dummy
        dummy->next = dummy;
        dummy->prev = dummy;

        allocationCnt = 0;
        deallocationCnt = 0;
        totalAllocatedSize = 0;
        totalDeallocatedSize = 0;
        enableAssert = true;
        lastError = NO_ERROR;

        current = this;
    }

    // no deinit func
    void deinit() {
        current = nullptr;

        // check DDL that only has dummy.
        ASSERT(dummy->prev == dummy && dummy->next == dummy && "Error: DDL is not empty, memory leak.");
        // delete dummy;
        free(dummy);
        dummy = nullptr;
    }

    void* allocation(size_t userSize, NEW_TYPE newType, const char* file = nullptr, int line = 0) {
        assert(newType == NEW_SINGLE || newType == NEW_ARRAY);  // internal error

        size_t totalSize = sizeof(Header) + userSize + tailSize;
        void* ptr = malloc(totalSize);
        if (!ptr) {
            return nullptr;
        }

        // fill the header
        Header* h = reinterpret_cast<Header*>(ptr);
        h->base = ptr;
        h->totalSize = totalSize;
        h->newType = newType;
        // h->newFile = file;  // 暂时没用到, 后面检查leak的时候可以把这个打印出来.
        // h->newLine = line;

        // fill the tail with some special chars.
        assert(tailSize == sizeof(uint32_t));  // internal error
        uint8_t* tail = reinterpret_cast<uint8_t*>(ptr);
        tail += sizeof(Header) + userSize;
        *reinterpret_cast<uint32_t*>(tail) = 0xDEADBEEF;

        insertToDDL(h);

        void* userPtr = reinterpret_cast<void*>(h + 1);
        allocationCnt++;
        totalAllocatedSize += totalSize;
        if (!enableAssert) cout << "[MemoryTracker allocation] base = " << ptr << ", userPtr = " << userPtr
                                << ", totalSize = (" << sizeof(Header) << "+" << userSize << "+" << tailSize << ") bytes\n";
        return userPtr;
    }

    void deallocation(void* userPtr, NEW_TYPE newType, size_t userSize = 0) {
        assert(newType == NEW_SINGLE || newType == NEW_ARRAY);  // internal error
        if (!userPtr) {
            return;
        }

        Header* h = reinterpret_cast<Header*>(userPtr) - 1;
        void* ptr = reinterpret_cast<void*>(h);
        size_t expectedUserSize = h->totalSize - sizeof(Header) - tailSize;

        // check the header
        ASSERT_E(h->base == ptr && "Error: the base of the total memory is not correct.", MEM_OVERRUN);
        // check the new type before the user size to return NEW_DELETE_MISMATCH error rather than MEM_LEAK error,
        // for mismatch testing.
        ASSERT_E(h->newType == newType && "new and delete mismatch.", NEW_DELETE_MISMATCH);
        if (userSize != 0) {
            // cout << "userSize = " << userSize << ", expectedUserSize = " << expectedUserSize << endl;
            ASSERT(userSize == expectedUserSize && "Error: user memory size is not correct.");
        }

        // check the tail content
        uint8_t* tail = reinterpret_cast<uint8_t*>(h);
        tail += sizeof(Header) + expectedUserSize;
        ASSERT_E(*reinterpret_cast<uint32_t*>(tail) == 0xDEADBEEF && "Warning: out of bounds memory access.", MEM_OVERRUN);

        if (!enableAssert) cout << "[MemoryTracker deallocation] base = " << ptr << ", userPtr = " << userPtr
                                << ", totalSize = (" << sizeof(Header) << "+" << expectedUserSize << "+" << tailSize << ") bytes\n";

        // record the info before freeing.
        deallocationCnt++;
        totalDeallocatedSize += h->totalSize;

        removeFromDDL(h);

        free(ptr);
    }

    // 下面的函数都是给mem tracker的测试程序用的.
    void setAssert(bool b) {
        enableAssert = b;
    }
    ERROR_TYPE getError() {
        ERROR_TYPE type = this->lastError;
        // clear the error after each query.
        lastError = NO_ERROR;
        return type;
    }
    size_t getAllocationCount() {
        return this->allocationCnt;
    }
    // 检查当前有没有leak, 然后reset回初始状态.
    bool hasLeak() {
        bool noLeak = dummy->prev == dummy &&
                      dummy->next == dummy &&
                      allocationCnt == deallocationCnt &&
                      totalAllocatedSize == totalDeallocatedSize;

        // free node memory
        while (dummy->next != dummy) {
            auto cur = dummy->next;
            removeFromDDL(cur);
            free(reinterpret_cast<void*>(cur));
        }
        deinit();
        init();

        return !noLeak;
    }
};

// 用这个NEW才能追踪到, 避免了追踪std里面的new/delete
// NEW和NEW[]都会用这个, 然后到对应的operator new()或者operator new[]()里面.
// #define NEW new (__FILE__, __LINE__)
// 如果定义下面的宏, 在call delete的时候要用: DELETE(p);
// #define DELETE(ptr) operator delete (ptr, __FILE__, __LINE__)
// 用下面的MyDeleter, 在call的时候就可以用 DELETE p;
// #define DELETE MyDeleter()->*

// inline void* doNew(size_t size, NEW_TYPE newType, const char* file, int line) {
//     // if enabled, use the MemoryTracker to malloc and track memory.
//     if (MemoryTracker::getCurrent()) {
//         return MemoryTracker::getCurrent()->allocation(size, newType, file, line);
//     }
//     return malloc(size);
// }

// inline void doDelete(void* ptr, NEW_TYPE newType, size_t size = 0) {
//     if (MemoryTracker::getCurrent()) {
//         MemoryTracker::getCurrent()->deallocation(ptr, newType, size);
//         return;
//     }
//     free(ptr);
// }

// size是编译器自带的, 后面两个file/line是通过"#define NEW new (__FILE__, __LINE__)"加上去的.
// inline void* operator new(size_t size, const char* file, int line) {
//     return doNew(size, NEW_SINGLE, file, line);
// }

// inline void operator delete(void* ptr, const char* file, int line) noexcept {
//     doDelete(ptr, NEW_SINGLE);

//     // call delete的文件的行信息不需要.
//     (void)file;
//     (void)line;
// }

// 重载 operator new[]
// inline void* operator new[](size_t size, const char* file, int line) {
//     return doNew(size, NEW_ARRAY, file, line);
// }

// // 重载 operator delete[], 这个用没有带size的版本
// inline void operator delete[](void* ptr) {
//     doDelete(ptr, NEW_ARRAY);
// }

//=========================================================
// 工具函数, 打印tid, 用printf的形式简单点,
// cout的形式要用类实现<<操作, 太麻烦了.
#define ENABLE_PRINT 1
#if ENABLE_PRINT
#define PRINTF(...)                            \
    do {                                       \
        stringstream ss;                       \
        ss << this_thread::get_id();           \
        printf("[tid=%s] ", ss.str().c_str()); \
        printf(__VA_ARGS__);                   \
        fflush(stdout);                        \
    } while (0)
#else  // ENABLE_PRINT
#define PRINTF(...)
#endif  // ENABLE_PRINT

#endif  // UTILS_H