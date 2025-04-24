#include <condition_variable>
#include <iostream>
#include <shared_mutex>
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
// 问题, 一般少量writer大量reader, writer可能一直抢不到锁, 一直在等, 被饿死
// 思路: 增加一个waiting_writers, 然写先拿到锁, 没人等着写, 读才能拿到锁. 
// 读完写完不要通知一个(通知一个如果读醒来, 看到有写在等,又睡去了,bug), 通知所有.
/*
还没跑过.

当有写者在等待时，新的 reader 会阻塞；

避免写者被后续不断到来的 reader “永远挤出”；

读性能略有下降，但更公平。
class rwlock {
    mutex mtx;
    condition_variable cv;
    int status;              // 0 = 空闲, >0 = reader 数量, -1 = 正在写
    int waiting_writers;     // 正在等待写锁的写线程数

public:
    rwlock() : status(0), waiting_writers(0) {}

    void readLock() {
        unique_lock<mutex> lck(mtx);
        cv.wait(lck, [this] {
            // 只有没人写，且没人等着写，才能读
            return status >= 0 && waiting_writers == 0;
        });
        status++;
    }

    void readUnlock() {
        unique_lock<mutex> lck(mtx);
        if (--status == 0) {
            cv.notify_all();  // 唤醒等待写的
        }
    }

    void writeLock() {
        unique_lock<mutex> lck(mtx);
        waiting_writers++;
        cv.wait(lck, [this] {
            return status == 0;
        });
        waiting_writers--;
        status = -1;
    }

    void writeUnlock() {
        unique_lock<mutex> lck(mtx);
        status = 0;
        cv.notify_all();  // 唤醒所有等待的（reader 也可能抢）
    }
};


*/
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

void Reader() {
    rwlk.readLock();

    PRINTF("read var: %d\n", var);
    this_thread::sleep_for(chrono::milliseconds(100));

    rwlk.readUnlock();
}

void Writer() {
    rwlk.writeLock();

    var++;
    PRINTF("write var: %d\n", var);
    this_thread::sleep_for(chrono::milliseconds(100));

    rwlk.writeUnlock();
}

void subtest1() {
    PRINTF("%s\n", __FUNCTION__);

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
}

//=========================================================
// C++17 现成的 read write lock

volatile int var2 = 0;  // 保持变量 var 对内存可见性，防止编译器过度优化
shared_mutex smtx2;     // 定义全局的读写锁变量, 当做普通的mutex来理解, 保护var

void Reader2() {
    shared_lock<shared_mutex> lck(smtx2);

    PRINTF("read var: %d\n", var);
    this_thread::sleep_for(chrono::milliseconds(100));
}

void Writer2() {
    unique_lock<shared_mutex> lck(smtx2);

    var++;
    PRINTF("write var: %d\n", var);
    this_thread::sleep_for(chrono::milliseconds(100));
}

void subtest2() {
    PRINTF("%s\n", __FUNCTION__);

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
}

int cppMain() {
    subtest1();
    subtest2();

    return 0;
}

}  // namespace thread_rwlock

/*===== Output =====

[RUN  ] thread_rwlock
[tid=1] subtest1
[tid=2] write var: 1
[tid=3] write var: 2
[tid=7] read var: 2
[tid=14] read var: 2
[tid=10] read var: 2
[tid=16] read var: 2
[tid=15] read var: 2
[tid=13] read var: 2
[tid=9] read var: 2
[tid=8] read var: 2
[tid=11] read var: 2
[tid=12] read var: 2
[tid=5] write var: 3
[tid=6] write var: 4
[tid=4] write var: 5
[tid=1] subtest2
[tid=17] write var: 6
[tid=21] write var: 7
[tid=18] write var: 8
[tid=30] read var: 8
[tid=26] read var: 8
[tid=31] read var: 8
[tid=29] read var: 8
[tid=25] read var: 8
[tid=23] read var: 8
[tid=28] read var: 8
[tid=22] read var: 8
[tid=24] read var: 8
[tid=27] read var: 8
[tid=19] write var: 9
[tid=20] write var: 10
[   OK] thread_rwlock

*/
