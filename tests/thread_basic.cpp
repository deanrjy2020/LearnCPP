#include <condition_variable>
#include <future>  // promise, future
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

class A {
private:
    int a;

public:
    A(int a) : a(a) {}
    void f4(int b) {
        PRINTF("a = %d, b = %d\n", a, b);
    }
};

void subtest1() {
    PRINTF("%s\n", __FUNCTION__);
    int n = 3;

    thread t1(f1);
    thread t2(f2, n - 1);  // pass by copy
    thread t3(f2, n);      // pass by ref

    // 如果调用的函数是在类里面，那么首先要在函数前面加上对应的类名，还要用取地址符，获取函数的地址传递过去
    // 第二个参数是类的实例化对象，如果这段代码是在类里面的化那么这个参数就是用this来代替。
    // 第三个参数是传递函数需要的参数。
    A a(1);
    thread t4(&A::f4, &a, 2);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
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

//=========================================================
// example 5, promise / future 101
// mutex 和 condition_variable 是用于线程同步的工具，
// 主要帮助管理共享资源的访问、避免数据竞争和在特定条件下进行线程的等待与通知。
// 而 std::future 和 std::promise 则用于线程之间的结果传递与同步.

void print_int(future<int>& fut) {
    // 在一个有效的 future 对象上调用 get 会阻塞当前的调用者，
    // 直到 Provider 设置了共享状态的值或异常（此时共享状态的标志变为 ready），
    // std::future::get 将返回异步任务的值或异常（如果发生了异常）。
    int x = fut.get();         // 获取共享状态的值.
    PRINTF("value: %d\n", x);  // 打印 value: 10.
}

void subtest5() {
    PRINTF("%s\n", __FUNCTION__);

    promise<int> prom;                    // 生成一个 promise<int> 对象.
    future<int> fut = prom.get_future();  // 和 future 关联.

    thread t(print_int, ref(fut));  // 将 future 交给另外一个线程t.
    PRINTF("setting value\n");
    prom.set_value(10);  // 设置共享状态的值, 此处和线程t保持同步.
    t.join();
}

int cppMain() {
    subtest1();
    subtest2();
    subtest3();
    subtest4();
    subtest5();
    return 0;
}

}  // namespace thread_basic

/*===== Output =====

[RUN  ] thread_basic
[tid=1] subtest1
[tid=2] f1
[tid=3] f2, n=2
[tid=4] f2, n=3
[tid=5] a = 1, b = 2
[tid=1] subtest2
[tid=6] 0 -> 1
[tid=7] 1 -> 2
[tid=8] 2 -> 3
[tid=6] 3 -> 4
[tid=7] 4 -> 5
[tid=8] 5 -> 6
[tid=8] 6 -> 7
[tid=6] 7 -> 8
[tid=7] 8 -> 9
[tid=7] 9 -> 10
[tid=8] 10 -> 11
[tid=6] 11 -> 12
[tid=8] 12 -> 13
[tid=7] 13 -> 14
[tid=6] 14 -> 15
15 successful increases of the counter2.
[tid=1] subtest3
[tid=9] 0 -> 1
[tid=11] 1 -> 2
[tid=10] 2 -> 3
[tid=10] 3 -> 4
[tid=9] 4 -> 5
[tid=11] 5 -> 6
[tid=10] 6 -> 7
[tid=9] 7 -> 8
[tid=11] 8 -> 9
[tid=11] 9 -> 10
[tid=10] 10 -> 11
[tid=9] 11 -> 12
[tid=10] 12 -> 13
[tid=9] 13 -> 14
[tid=11] 14 -> 15
15 successful increases of the counter3.
[tid=1] subtest4
10 threads ready to race...
[tid=14] worker awake
[tid=16] worker awake
[tid=13] worker awake
[tid=19] worker awake
[tid=15] worker awake
[tid=17] worker awake
[tid=12] worker awake
[tid=18] worker awake
[tid=20] worker awake
[tid=21] worker awake
[tid=1] subtest5
[tid=1] setting value
[tid=22] value: 10
[   OK] thread_basic

*/
