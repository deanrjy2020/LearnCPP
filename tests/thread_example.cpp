#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
using namespace std;

#include "utils.h"

namespace thread_example {

//=========================================================
// https://blog.csdn.net/liuxuejiang158blog/article/details/21977009
// 题目：子线程循环 1 次，接着主线程循环 2 次，接着又回到子线程循环 1 次，
// 接着再回到主线程又循环 2 次，如此循环5次，试写出代码。
// idea:
// 用一个unique_lock/mutex和condition_variable
// 两个线程用同一个入口函数, 参数为loopNum 1 or 2.
// 用一个flag, 初始为1, 代表loopNum为1的先开始, 不是的就wait, flag在1和2互换.
//
// https://blog.csdn.net/liuxuejiang158blog/article/details/22061267
// 题目：编写一个程序，开启3个线程，这3个线程的ID分别为A、B、C，每个线程将自己
// 的ID在屏幕上打印10遍，要求输出结果必须按ABC的顺序显示；如：ABCABC….依次递推。
// 和上面一样的思路
//
// https://blog.csdn.net/liuxuejiang158blog/article/details/22105455
// 题目：有四个线程1、2、3、4。线程1的功能就是输出1，线程2的功能就是输出2，
// 以此类推.........现在有四个文件ABCD。初始都为空。现要让四个文件呈如下格式：
// A：1 2 3 4 1 2....
// B：2 3 4 1 2 3....
// C：3 4 1 2 3 4....
// D：4 1 2 3 4 1....
// 还是和上面一样的思路, 链接中是把文件名当参数传入main, 程序跑4次, 每次写一个文件, 这样程序简单.
// 也可以一个线程写入4个文件, 这样打开关闭文件频繁, 好像不好.

class A1 {
private:
    mutex mtx;
    condition_variable cv;
    int curNum;

public:
    A1(int n) : curNum(n) {}

    void foo(int num) {
        for (int i = 0; i < 5; ++i) {
            unique_lock<mutex> lck(mtx);
            // ready = 是当前的loop num了.
            cv.wait(lck, [this, num] { return curNum == num; });
            for (int i = 0; i < num; ++i) {
                cout << (num == 1 ? "!" : "@");
            }
            // 互换, 然后通知另一个
            curNum = (num == 1 ? 2 : 1);
            cv.notify_one();
        }
    }
};

void subtest1() {
    PRINTF("%s\n", __FUNCTION__);

    A1 a1(1);  // 从子线程开始打
    thread t(&A1::foo, &a1, 1);
    a1.foo(2);
    t.join();
    cout << endl;
}

//=========================================================
// 线程安全的queue
// STL中的queue是非线程安全的，一个组合操作：front(); pop()先读取
// 队首元素然后删除队首元素，若是有多个线程执行这个组合操作的话，
// 可能会发生执行序列交替执行，导致一些意想不到的行为。因此需要重新设计线程安全的queue的接口。
// idea:
// 用一个unique_lock/mutex保护queue, cv通知其他线程
// 1, 每次对queue操作(读写)的时候lock
// push不用等, 完了要通知, 有可能其他在等着
// 2, pop如果是空的要等
// 3, 不加capacity了, 加了的话push也要等, 还要另外一个cv
// 4, 测试略

template <typename T>
class BlockingQueue {
private:
    queue<T> q;
    // size_t capacity;
    mutex mtx;
    condition_variable cv;

public:
    // BlockingQueue(size_t capa) : capacity(capa) {}
    //~BlockingQueue() {}
    // 因为有mutex和condition_variable类成员, 所以此类不支持复制构造函数也不支持赋值操作符(=)
    BlockingQueue(const BlockingQueue& other) = delete;
    BlockingQueue& operator=(const BlockingQueue& other) = delete;

    void push(const T& item) {
        unique_lock<mutex> lck(mtx);
        q.push(item);
        lck.unlock();
        cv.notify_all();
    }
    void pop() {
        unique_lock<mutex> lck(mtx);
        // ready = not empty
        cv.wait(lck, [&] { return !q.empty(); });
        q.pop();
    }
    T front() {
        unique_lock<mutex> lck(mtx);
        assert(!q.empty());  // user's respon
        return q.front();
    }
    bool empty() {
        unique_lock<mutex> lck(mtx);
        return q.empty();
    }
    size_t size() {
        unique_lock<mutex> lck(mtx);
        return q.size();
    }
};

//=========================================================
void subtest2() {
    PRINTF("%s\n", __FUNCTION__);
}
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================
//=========================================================

int cppMain() {
    subtest1();
    subtest2();

    return 0;
}

}  // namespace thread_example

/*===== Output =====

[RUN  ] thread_example
[tid=1] subtest1
!@@!@@!@@!@@!@@
[tid=1] subtest2
[Memory Report] allocationCnt = deallocationCnt = 1, totalAllocatedSize = totalDeallocatedSize = 84 Bytes
[   OK] thread_example

*/
