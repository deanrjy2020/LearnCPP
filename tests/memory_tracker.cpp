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
    // MemoryTracker*& current = MemoryTracker::getCurrent();
    // assert(current);
    // assert(current->hasLeak() == false);
    // // disable assert, use the log instead.
    // current->setAssert(false);

    size_t testId = 0;

    {
        cout << "===test" << testId++ << ": multithread.\n";
        std::vector<std::thread> threads;
        for (int i = 0; i < 1; ++i)
            threads.emplace_back([] {
                int* p = new int(33);
                delete p;
            });
        for (auto& t : threads)
            t.join();
    }

    {
        // 这种情况MemoryTracker应该发现不了
        // 同时如果用户跳过了0xDEADBEEF位置, 在后面写, MemoryTracker也发现不了
        cout << "===test" << testId++ << ": no warning.\n";
        int* a = new int(33);
        *(a + 1) = 0xDEADBEEF;
        delete a;
        // assert(current->getAllocationCount() == testId && current->getError() == NO_ERROR);
    }

    {
        cout << "===test" << testId++ << ": use new[] and no delete[], hasLeak.\n";
        int* a = new int[3];
        delete[] a;
        // assert(current->getError() == NO_ERROR);
        // assert(current->hasLeak());
        // current->setAssert(false);
    }

    {
        cout << "===test" << testId++ << ": also track the new/delete in std.\n";
        vector<int> vec;
        vec.push_back(39);
        // allocation count 是累加的, current用的是同一个.
        // assert(current->getAllocationCount() == testId);
    }
#if 0
    {
        cout << "===test" << testId++ << ": use new[] and no delete[], hasLeak.\n";
        int* a = new int[3];
        (void)a;
        // assert(current->getError() == NO_ERROR);
        // assert(current->hasLeak());
        // current->setAssert(false);
    }



    // {
    //     cout << "===test" << testId++ << ": use new and no delete, hasLeak.\n";
    //     int* a = new int(37);
    //     (void)a;
    //     // assert(current->getError() == NO_ERROR);
    //     // assert(current->hasLeak());
    //     // current->setAssert(false);
    // }



    // 如果把header改掉了, 就会把里面的totalSize改掉, 会crash, 测试没什么意义, skip.
    // {
    //     cout << "test: single new, modify header and trigger the MEM_OVERRUN\n";
    //     int* a = new int(34);
    //     *(a - 1) = 0xDEADBEEF;
    //     delete a;
    //     assert(current->getError() == MEM_OVERRUN);
    // }

    // {
    //     cout << "===test" << testId++ << ": single new, modify the tail and trigger the MEM_OVERRUN.\n";
    //     int* a = new int(34);
    //     *(a + 1) = 77;
    //     //assert(current->getError() == NO_ERROR);
    //     delete a;
    //     //assert(current->getAllocationCount() == testId && current->getError() == MEM_OVERRUN);
    // }

    // {
    //     cout << "===test" << testId++ << ": array new, modify the tail and trigger the MEM_OVERRUN.\n";
    //     int* a = new int[35];
    //     *(a + 35) = 78;
    //     assert(current->getError() == NO_ERROR);
    //     delete[] a;
    //     assert(current->getError() == MEM_OVERRUN);
    // }

    // new (v15, v8.1 won't) gcc compiler will error out when mismatch, don't need to test.
    // {
    //     cout << "===test" << testId++ << ": mismatch, use new and delete[], trigger the NEW_DELETE_MISMATCH.\n";
    //     int* a = new int(36);
    //     assert(current->getError() == NO_ERROR);
    //     delete[] a;
    //     assert(current->getError() == NEW_DELETE_MISMATCH);
    // }

    // {
    //     cout << "===test" << testId++ << ": mismatch, use new[] and delete, trigger the NEW_DELETE_MISMATCH.\n";
    //     int* a = new int[2];
    //     assert(current->getError() == NO_ERROR);
    //     delete a;
    //     assert(current->getError() == NEW_DELETE_MISMATCH);
    //     assert(current->hasLeak());
    //     current->setAssert(false);
    // }

#endif
    // other testing:
    // delete ptr twice?
    // userPtr is 10, but user passed 12 ?

    cout << "cppMain done\n";
    return 0;
}

}  // namespace memory_tracker

/*===== Output =====

[RUN  ] memory_tracker
===test0: multithread.
===test1: no warning.
===test2: use new[] and no delete[], hasLeak.
===test3: also track the new/delete in std.
cppMain done
[tid=1] [Memory Report] globalNewCnt = 6, globalDeleteCnt = 6, globalNewMemSize = 312, globalDeleteMemSize = 312
[   OK] memory_tracker

*/
