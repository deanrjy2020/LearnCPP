#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
using namespace std;

#include "utils.h"

namespace impl_semaphore {

//=========================================================
// 用mutex+condition_variable实现semaphore

class Semaphore {
private:
    mutex mtx;
    condition_variable cv;
    int count;  // 表示当前“资源数量”。

public:
    Semaphore(int cnt) : count(cnt) {}
    // 因为有mutex和condition_variable类成员, 所以此类不支持复制构造函数也不支持赋值操作符(=)
    Semaphore(const Semaphore& other) = delete;
    Semaphore& operator=(const Semaphore& other) = delete;

    void wait() {
        unique_lock<mutex> lck(mtx);
        // ready = count>0, 表示有资源.
        cv.wait(lck, [&] { return count > 0; });
        count--;
    }

    void signal() {
        unique_lock<mutex> lck(mtx);
        count++;
        cv.notify_one();
    }
};

Semaphore sem(3);  // 最多允许3个线程同时访问

void worker(int id) {
    sem.wait();
    PRINTF("Thread %d is working.\n", id);
    this_thread::sleep_for(chrono::seconds(rand() % 2));  // 0~2s随机数
    PRINTF("Thread %d is done.\n", id);
    sem.signal();
}

int cppMain() {
    vector<thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}

}  // namespace impl_semaphore

/*===== Output =====

[RUN  ] impl_semaphore
[tid=2] Thread 0 is working.
[tid=3] Thread 1 is working.
[tid=4] Thread 2 is working.
[tid=4] Thread 2 is done.
[tid=3] Thread 1 is done.
[tid=5] Thread 3 is working.
[tid=6] Thread 4 is working.
[tid=2] Thread 0 is done.
[tid=8] Thread 6 is working.
[tid=5] Thread 3 is done.
[tid=6] Thread 4 is done.
[tid=7] Thread 5 is working.
[tid=9] Thread 7 is working.
[tid=8] Thread 6 is done.
[tid=10] Thread 8 is working.
[tid=10] Thread 8 is done.
[tid=9] Thread 7 is done.
[tid=11] Thread 9 is working.
[tid=7] Thread 5 is done.
[tid=11] Thread 9 is done.
[tid=1] [Memory Report] globalNewCnt = 15, globalDeleteCnt = 15, globalNewMemSize = 1148, globalDeleteMemSize = 1148
[   OK] impl_semaphore

*/
