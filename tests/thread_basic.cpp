#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
using namespace std;

#include "utils.h"

namespace thread_basic {

//=========================================================
// example 1, hello world

void f1() {
    PRINTF("f1\n");
}
void f2(int n) {
    PRINTF("f2, n=%d\n", n);
}
void f3(int& n) {
    PRINTF("f3, n=%d\n", n);
}

void subtest1() {
    PRINTF("%s\n", __FUNCTION__);
    int n = 3;

    thread t1(f1);
    thread t2(f2, n - 1);  // pass by copy
    thread t3(f2, n);      // pass by ref

    t1.join();
    t2.join();
    t3.join();
}

//=========================================================
// example 2, mutex

// 全局变量后面数字为example id, 避免冲突.
volatile int counter2(0);  // non-atomic counter2
mutex mtx2;                // locks access to counter2

void attempt_5_increases_v2() {
    for (int i = 0; i < 5; ++i) {
        mtx2.lock();
        PRINTF("%d -> %d\n", counter2, counter2 + 1);
        ++counter2;
        mtx2.unlock();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void subtest2() {
    PRINTF("%s\n", __FUNCTION__);

    thread threads[3];
    for (int i = 0; i < 3; ++i) {
        threads[i] = thread(attempt_5_increases_v2);
    }
    for (auto& th : threads) {
        th.join();
    }
    cout << counter2 << " successful increases of the counter2.\n";
}

//=========================================================
// example 3, unique_lock
// 和example 2 mutex完全一样的功能, 除了用unique_lock<mutex>而不是单单用mutex

volatile int counter3(0);  // non-atomic counter3
mutex mtx3;                // locks access to counter3

void attempt_5_increases_v3() {
    for (int i = 0; i < 5; ++i) {
        unique_lock<mutex> lck(mtx3);  // 默认创建就lock
        PRINTF("%d -> %d\n", counter3, counter3 + 1);
        ++counter3;
        lck.unlock();  // 可以自动unlock, 这里为了后面的sleep, 手动解锁.
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

void subtest3() {
    PRINTF("%s\n", __FUNCTION__);

    thread threads[3];
    for (int i = 0; i < 3; ++i) {
        threads[i] = thread(attempt_5_increases_v3);
    }
    for (auto& th : threads) {
        th.join();
    }
    cout << counter3 << " successful increases of the counter3.\n";
}

//=========================================================
// example 4, condition_variable 101
// 记住, cv就是要定义什么是ready状态.

mutex mtx4;  // 全局互斥锁, 保护ready4, 任何改ready4的地方都要lock
condition_variable cv4;
bool ready4 = false;

void worker() {
    // 当前线程自动上锁
    unique_lock<mutex> lck(mtx4);
    // ready==false, 当前线程被阻塞, 等待 ready==true
    // 被阻塞时, wait会自动调用lck.unlock()释放锁, 使得其他被阻塞在锁竞争上的线程得以继续执行,
    // 一旦当前线程获得通知(notified，通常是另外某个线程调用 notify_* 唤醒了当前线程)，
    // wait()函数也是自动调用 lck.lock()，使得lck的状态和 wait 函数被调用时相同.
    cv4.wait(lck, [] { return ready4; });
    // 这里持有锁并且 ready==true
    // 线程被唤醒, 开始工作
    PRINTF("worker awake\n");
}

void go() {
    unique_lock<mutex> lck(mtx4);
    ready4 = true;     // 设置全局标志位为 true.
    cv4.notify_all();  // 唤醒所有线程.
}

void subtest4() {
    PRINTF("%s\n", __FUNCTION__);

    thread threads[10];
    for (int i = 0; i < 10; ++i) {
        threads[i] = thread(worker);
    }

    cout << "10 threads ready to race...\n";
    go();
    for (auto& th : threads) {
        th.join();
    }
}

int cppMain() {
    subtest1();
    subtest2();
    subtest3();
    subtest4();
    return 0;
}

}  // namespace thread_basic

/*===== Output =====

[RUN  ] thread_basic
[tid=1] subtest1
[tid=2] f1
[tid=3] f2, n=2
[tid=4] f2, n=3
[tid=1] subtest2
[tid=5] 0 -> 1
[tid=6] 1 -> 2
[tid=7] 2 -> 3
[tid=7] 3 -> 4
[tid=6] 4 -> 5
[tid=5] 5 -> 6
[tid=7] 6 -> 7
[tid=5] 7 -> 8
[tid=6] 8 -> 9
[tid=7] 9 -> 10
[tid=6] 10 -> 11
[tid=5] 11 -> 12
[tid=5] 12 -> 13
[tid=6] 13 -> 14
[tid=7] 14 -> 15
15 successful increases of the counter2.
[tid=1] subtest3
[tid=8] 0 -> 1
[tid=9] 1 -> 2
[tid=10] 2 -> 3
[tid=10] 3 -> 4
[tid=9] 4 -> 5
[tid=8] 5 -> 6
[tid=8] 6 -> 7
[tid=9] 7 -> 8
[tid=10] 8 -> 9
[tid=10] 9 -> 10
[tid=8] 10 -> 11
[tid=9] 11 -> 12
[tid=8] 12 -> 13
[tid=9] 13 -> 14
[tid=10] 14 -> 15
15 successful increases of the counter3.
[tid=1] subtest4
10 threads ready to race...
[tid=15] worker awake
[tid=14] worker awake
[tid=12] worker awake
[tid=11] worker awake
[tid=18] worker awake
[tid=13] worker awake
[tid=17] worker awake
[tid=16] worker awake
[tid=19] worker awake
[tid=20] worker awake
[   OK] thread_basic

*/
