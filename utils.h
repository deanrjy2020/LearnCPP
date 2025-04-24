#ifndef UTILS_H
#define UTILS_H

#include <math.h>

#include <cassert>
#include <mutex>
#include <sstream>
#include <thread>

//=========================================================

inline bool nearlyEqual(float a, float b, float epsilon = 1e-5f) {
    return fabs(a - b) < epsilon;
}

//=========================================================
// 工具函数, 打印tid, 用printf的形式简单点,
// cout的形式要用类实现<<操作, 太麻烦了.

// 用run.sh脚本跑的时候log会乱序, 加上锁. 直接跑没有乱序
static mutex logMtx;

#define ENABLE_PRINT 1
#if ENABLE_PRINT
#define PRINTF(...)                            \
    do {                                       \
        lock_guard<mutex> lock(logMtx);        \
        stringstream ss;                       \
        ss << this_thread::get_id();           \
        printf("[tid=%s] ", ss.str().c_str()); \
        printf(__VA_ARGS__);                   \
        fflush(stdout);                        \
    } while (0)
#else
#define PRINTF(...)
#endif

//=========================================================
// MemoryTracker结合全局operator new/delete
// 功能:
// - 检测mem leak
// - 检测mem overrun (如果跳过了tail无法检测到)
// - 检测new/delete mismatch, e.g. new + delete[], 不用检测, 编译会报错.
// - 不支持reallocate这种操作
// - 不支持mem size的alignment, 追踪精确的size, 不做.
// - 不支持mem addr的alignment, 这个也会导致size变化, 不做. 单单addr的align见mem_addr_align.cpp
// 实现:
// - 前面加头, 后面加尾巴, 检测用户拿到/返回的mem是否有异常
// - 用doubly linked list实现
// - 线程安全
//       方案1, 用一个mutex, allocation和deallocation都加锁, 但是这样性能会下降, 因为每次内存分配/释放都需要加锁.
//       方案2, 更细粒度, 用一个mutex保护DLL, 统计数据如allocationCnt用atomic, 缺点在改动DLL是还是串行的, 在高并发场景下，DDL 的锁竞争会成为性能瓶颈
//     x 方案3, thread_local, 每个线程一个tracker instance. 线程退出的时候会merge结果到global.
// todo:
// 通过所有的tests, 目前还有bug.
// 在DLL实现了已经, 在用hash map实现, 双重检测., 这个在解决下面的问题后再说.
// GPT to review

/*
用上下面这些:
做negative test
-enum ERROR_TYPE {
-    NO_ERROR,
-    MEM_LEAK,
-    MEM_OVERRUN,  // out of bounds memory access.
-    NEW_DELETE_MISMATCH,
-};

 */

enum NEW_TYPE {
    NEW_NONE,
    NEW_SINGLE,
    NEW_ARRAY,
};

static bool debug = false;
class MemoryTracker {
private:
    struct Header {
        Header* prev;
        Header* next;
        void* base;
        size_t totalSize;
        NEW_TYPE newType;
        Header() : prev(nullptr), next(nullptr), base(nullptr), totalSize(0), newType(NEW_NONE) {}
    };

    struct GlobalStats {
        mutex mtx;
        size_t globalNewCnt = 0;
        size_t globalDeleteCnt = 0;
        size_t globalNewMemSize = 0;
        size_t globalDeleteMemSize = 0;
        void dumpSummary() const {
            // 如果MemoryTracker被disable, 则不打印.
            if (trackingEnabled) {
                PRINTF("[Memory Report] globalNewCnt = %zu, globalDeleteCnt = %zu, globalNewMemSize = %zu, globalDeleteMemSize = %zu\n",
                       globalNewCnt, globalDeleteCnt, globalNewMemSize, globalDeleteMemSize);
            }
            assert(globalNewCnt == globalDeleteCnt && globalNewMemSize == globalDeleteMemSize);
        }
    };

    struct ThreadStats {
        Header dummy;
        size_t localNewCnt = 0;
        size_t localDeleteCnt = 0;
        size_t localNewMemSize = 0;
        size_t localDeleteMemSize = 0;
        // bool localTouched = false;

        ThreadStats() {
            dummy.prev = &dummy;
            dummy.next = &dummy;
        }
        ~ThreadStats() {
            // PRINTF("~ThreadStats, localTouched = %d\n", localTouched);
            // if (trackingEnabled && localTouched) {
            if (trackingEnabled) {
                if (debug) {
                    PRINTF("worker thread summary: localNewCnt = %zu, localDeleteCnt = %zu, localNewMemSize = %zu, localDeleteMemSize = %zu\n ",
                           localNewCnt, localDeleteCnt, localNewMemSize, localDeleteMemSize);
                }
                mergeTo(global_stats);
            }
        }
        void insertToDDL(Header*& h) {
            h->next = dummy.next;
            h->prev = &dummy;
            dummy.next = h;
            h->next->prev = h;
        }
        void removeFromDDL(Header*& h) {
            h->next->prev = h->prev;
            h->prev->next = h->next;
            h->next = nullptr;
            h->prev = nullptr;
        }
        // bool hasLeak() const {
        //     return dummy.next != &dummy;
        // }
        void mergeTo(GlobalStats& global) {
            lock_guard<mutex> lock(global.mtx);
            global.globalNewCnt += localNewCnt;
            global.globalDeleteCnt += localDeleteCnt;
            global.globalNewMemSize += localNewMemSize;
            global.globalDeleteMemSize += localDeleteMemSize;
        }
    };

    // static thread_local ThreadLocalStats tls_stats_holder;
    static thread_local ThreadStats tls_stats;
    static GlobalStats global_stats;
    static bool trackingEnabled;

    // #define tls_stats (tls_stats_holder.stats)

public:
    class Scope {
    public:
        Scope(bool enable = true) {
            trackingEnabled = enable;
        }

        ~Scope() {
            if (debug) {
                PRINTF("main thread summary: localNewCnt = %zu, localDeleteCnt = %zu, localNewMemSize = %zu, localDeleteMemSize = %zu\n",
                       tls_stats.localNewCnt, tls_stats.localDeleteCnt, tls_stats.localNewMemSize, tls_stats.localDeleteMemSize);
            }
            tls_stats.mergeTo(global_stats);

            global_stats.dumpSummary();
            trackingEnabled = false;
        }
    };

    static void* allocate(size_t userSize, NEW_TYPE newType);
    static void deallocate(void* userPtr, NEW_TYPE newType, size_t userSize = 0);
};

// 重载全局operator new/delete
inline void* operator new(size_t size) { return MemoryTracker::allocate(size, NEW_SINGLE); }
inline void operator delete(void* ptr, size_t size) noexcept { MemoryTracker::deallocate(ptr, NEW_SINGLE, size); }
inline void* operator new[](size_t size) { return MemoryTracker::allocate(size, NEW_ARRAY); }
inline void operator delete[](void* ptr) noexcept { MemoryTracker::deallocate(ptr, NEW_ARRAY); }

// 用这个NEW才能追踪到, 避免了追踪std里面的new/delete
// NEW和NEW[]都会用这个, 然后到对应的operator new()或者operator new[]()里面.
// #define NEW new (__FILE__, __LINE__)
// 如果定义下面的宏, 在call delete的时候要用: DELETE(p);
// #define DELETE(ptr) operator delete (ptr, __FILE__, __LINE__)
// 用下面的MyDeleter, 在call的时候就可以用 DELETE p;
// #define DELETE MyDeleter()->*

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
// 重载 operator delete[], 这个用没有带size的版本
// inline void operator delete[](void* ptr) {
//     doDelete(ptr, NEW_ARRAY);
// }

#endif  // UTILS_H
