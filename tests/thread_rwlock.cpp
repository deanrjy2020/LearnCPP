#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

#include "utils.h"

namespace thread_rwlock {

//=========================================================
// condition_variable实现rwlock
// 接口:
// class RWLock {
// public:
// void lock_read();    // 获取读锁
// void unlock_read();  // 释放读锁
// void lock_write();   // 获取写锁
// void unlock_write(); // 释放写锁
// };
// lock_read：多个读线程可以并发拿到；
// lock_write：写线程需要独占，所有人（读+写）都要等。
//

class rwlock {
    mutex mtx;              // 用来保护status
    condition_variable cv;  // 通知其他thread去读或者写
    // 0, 没有锁, >0, reader的个数, -1, 已加写锁
    int status;

public:
    rwlock() : status(0) {}

    void readLock() {
        unique_lock<mutex> lck(mtx);
        cv.wait(lck, [this] {
            // 没有人在写就是ready
            return this->status != -1;
        });
        // 如果有人在读, 没关系, 加入进去读
        // 没人读, 直接开始
        status++;
    }

    void readUnlock() {
        unique_lock<mutex> lck(mtx);
        if (--status == 0) {
            // 不可能别人在等着读, 只能等着写(可能多个), 通知一个写的就行了.
            // 通知全部了也只能有一个来写, 用all也可以
            cv.notify_one();
        }
    }

    void writeLock() {
        unique_lock<mutex> lck(mtx);
        cv.wait(lck, [this] {
            // 没人读没人写才是ready
            return this->status == 0;
        });
        status = -1;
    }

    void writeUnlock() {
        unique_lock<mutex> lck(mtx);
        status = 0;
        // 叫醒所有等待的读和写操作, 可能多个读就一起了.
        cv.notify_all();
    }
};

volatile int var = 0;  // 保持变量 var 对内存可见性，防止编译器过度优化
rwlock rwlk;           // 定义全局的读写锁变量, 保护var

void Writer() {
    rwlk.writeLock();
    var++;
    PRINTF("write var: %d\n", var);
    rwlk.writeUnlock();
}

void Reader() {
    rwlk.readLock();
    PRINTF("read var: %d\n", var);
    rwlk.readUnlock();
}

int cppMain() {
    vector<thread> readers, writers;
    for (int i = 0; i < 5; ++i) {  // 5 个写线程
        writers.emplace_back(Writer);
    }
    for (int i = 0; i < 10; ++i) {  // 10 个读线程
        readers.emplace_back(Reader);
    }

    for (auto& w : writers) {
        w.join();
    }
    for (auto& r : readers) {
        r.join();
    }

    return 0;
}

}  // namespace thread_rwlock

/*===== Output =====

[RUN  ] thread_rwlock
[tid=2] write var: 1
[tid=4] write var: 2
[tid=3] write var: 3
[tid=5] write var: 4
[tid=7] read var: 4
[tid=8] read var: 4
[tid=16] read var: 4
[tid=9] read var: 4
[tid=11] read var: 4
[tid=13] read var: 4
[tid=12] read var: 4
[tid=14] read var: 4
[tid=6] write var: 5
[tid=10] read var: 5
[tid=15] read var: 5
[   OK] thread_rwlock

*/
