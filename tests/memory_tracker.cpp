#include <iostream>
#include <vector>
using namespace std;

#include "utils.h"  // MemoryTracker
// bool MemoryTracker::enableAssert = false;
// MemoryTracker* MemoryTracker::current;

namespace memory_tracker {

//=========================================================
// memory_tracker实现在utils.h里面, 结合全局operator new/delete
// 每一个test只要用了new/delete都是测试, 只是没有MemoryTracker类
// 里的详细log打印出来(默认disable), 只有Memory Report log.
// 这里是一些额外的测试, 如negative test(故意内存越界访问)
// 同时enable log

int cppMain() {
    // use the global mem tracker to test.
    MemoryTracker*& current = MemoryTracker::getCurrent();
    assert(current);
    assert(current->hasLeak() == false);
    // disable assert, use the log instead.
    current->setAssert(false);

    size_t testId = 0;

    {
        // 这种情况MemoryTracker应该发现不了
        // 同时如果用户跳过了0xDEADBEEF位置, 在后面写, MemoryTracker也发现不了
        cout << "===test" << testId++ << ": no warning.\n";
        int* a = new int(33);
        *(a + 1) = 0xDEADBEEF;
        delete a;
        assert(current->getAllocationCount() == testId && current->getError() == NO_ERROR);
    }

    // 如果把header改掉了, 就会把里面的totalSize改掉, 会crash, 测试没什么意义, skip.
    // {
    //     cout << "test: single new, modify header and trigger the MEM_OVERRUN\n";
    //     int* a = new int(34);
    //     *(a - 1) = 0xDEADBEEF;
    //     delete a;
    //     assert(current->getError() == MEM_OVERRUN);
    // }

    {
        cout << "===test" << testId++ << ": single new, modify the tail and trigger the MEM_OVERRUN.\n";
        int* a = new int(34);
        *(a + 1) = 77;
        assert(current->getError() == NO_ERROR);
        delete a;
        assert(current->getAllocationCount() == testId && current->getError() == MEM_OVERRUN);
    }

    {
        cout << "===test" << testId++ << ": array new, modify the tail and trigger the MEM_OVERRUN.\n";
        int* a = new int[35];
        *(a + 35) = 78;
        assert(current->getError() == NO_ERROR);
        delete[] a;
        assert(current->getError() == MEM_OVERRUN);
    }

    {
        cout << "===test" << testId++ << ": mismatch, use new and delete[], trigger the NEW_DELETE_MISMATCH.\n";
        int* a = new int(36);
        assert(current->getError() == NO_ERROR);
        delete[] a;
        assert(current->getError() == NEW_DELETE_MISMATCH);
    }

    {
        cout << "===test" << testId++ << ": mismatch, use new[] and delete, trigger the NEW_DELETE_MISMATCH.\n";
        int* a = new int[2];
        assert(current->getError() == NO_ERROR);
        delete a;
        assert(current->getError() == NEW_DELETE_MISMATCH);
        assert(current->hasLeak());
        current->setAssert(false);
    }

    // backup global mem tracker
    // MemoryTracker* globalTracker = current;
    // current = nullptr;
    // (void)globalTracker;

    {
        cout << "===test" << testId++ << ": use new and no delete, hasLeak.\n";
        int* a = new int(37);
        (void)a;
        assert(current->getError() == NO_ERROR);
        assert(current->hasLeak());
        current->setAssert(false);
    }

    {
        cout << "===test" << testId++ << ": use new[] and no delete[], hasLeak.\n";
        int* a = new int[3];
        (void)a;
        assert(current->getError() == NO_ERROR);
        assert(current->hasLeak());
        current->setAssert(false);
    }

    // {
    //     cout << "===test" << testId++ << ": don't track the new/delete in std.\n";
    //     // int* a = new int(38);
    //     int* a = NEW int(38);
    //     vector<int> vec;
    //     vec.push_back(*a);
    //     DELETE a;
    //     assert(current->getAllocationCount() == 1);
    // }

    // other testing:
    // delete ptr twice?
    // userPtr is 10, but user passed 12 ?

    cout << "cppMain done\n";
    return 0;
}

}  // namespace memory_tracker

/*===== Output =====

[RUN  ] memory_tracker
===test0: no warning.
[MemoryTracker allocation] base = 0x27d7340, userPtr = 0x27d7368, totalSize = (40+4+4) bytes
[MemoryTracker deallocation] base = 0x27d7340, userPtr = 0x27d7368, totalSize = (40+4+4) bytes
===test1: single new, modify the tail and trigger the MEM_OVERRUN.
[MemoryTracker allocation] base = 0x27d7340, userPtr = 0x27d7368, totalSize = (40+4+4) bytes
Assertion failed:
  Expression: *reinterpret_cast<uint32_t*>(tail) == 0xDEADBEEF && "Warning: out of bounds memory access.", File: utils.h, Line: 221
===test2: array new, modify the tail and trigger the MEM_OVERRUN.
[MemoryTracker allocation] base = 0x27d7380, userPtr = 0x27d73a8, totalSize = (40+140+4) bytes
Assertion failed:
  Expression: *reinterpret_cast<uint32_t*>(tail) == 0xDEADBEEF && "Warning: out of bounds memory access.", File: utils.h, Line: 221
===test3: mismatch, use new and delete[], trigger the NEW_DELETE_MISMATCH.
[MemoryTracker allocation] base = 0x27d7440, userPtr = 0x27d7468, totalSize = (40+4+4) bytes
Assertion failed:
  Expression: h->newType == newType && "new and delete mismatch.", File: utils.h, Line: 212
===test4: mismatch, use new[] and delete, trigger the NEW_DELETE_MISMATCH.
[MemoryTracker allocation] base = 0x27d7480, userPtr = 0x27d74a8, totalSize = (40+8+4) bytes
Assertion failed:
  Expression: h->newType == newType && "new and delete mismatch.", File: utils.h, Line: 212
===test5: use new and no delete, hasLeak.
[MemoryTracker allocation] base = 0x27d7340, userPtr = 0x27d7368, totalSize = (40+4+4) bytes
===test6: use new[] and no delete[], hasLeak.
[MemoryTracker allocation] base = 0x27d7340, userPtr = 0x27d7368, totalSize = (40+12+4) bytes
cppMain done
[Memory Report] allocationCnt = deallocationCnt = 0, totalAllocatedSize = totalDeallocatedSize = 0 Bytes
[   OK] memory_tracker

*/
