#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>
using namespace std;

#include "utils.h"

namespace thread_prod_cons {

//=========================================================
// condition_variable实现producer/consumer
// 思想:
// 多个producer, 多个consumer, 一个queue.
// 全部producer共用一个入口函数, 全部consumer共用一个入口函数
//
// 共2个cv,
// producer等一个prod_cv, queue里面不是满的就是ready
// consumer等一个cons_cv, queue里面不是空的就是ready

// 下面不是const的都是critical section, 要lock再改值.
queue<int> q;
const int capacity = 10;
mutex mtx;
// producer cv, producer会等着这个cv.
// 只要q不是full, producer就可干活, 即ready.
condition_variable prod_cv;
// consumer cv, consumer会等着这个cv.
// 只要q不是empty, consumer就可干活, 即ready.
condition_variable cons_cv;

// 这里只是一种终止的方法, 也可以让其跑一定时间, 然后停止.
// 改动下面producing/consuming的时间, 处理快的就会多sleep
const int totalItems = 30;
int producedCnt = 0;  // item id 也用这个, 简单起见, 不要把prod id放到item id里面
int consumedCnt = 0;

void producer(int id) {
    PRINTF("producer%d\n", id);
    while (true) {
        // producer起来第一件事情不是拿锁, 而是先生产item.
        // similate for producing the item.
        // item的存取一般认为是比较快的, 重点在于做好同步,
        // 生成和处理是比较花时间的.
        this_thread::sleep_for(chrono::milliseconds(100));

        unique_lock<mutex> lck(mtx);
        prod_cv.wait(lck, [id] {
            // q is not full, ready.
            // or produced enough items => wake up to quit
            if (q.size() < capacity || producedCnt == totalItems) {
                return true;
            }
            PRINTF("q is full, producer%d sleep...\n", id);
            return false;
        });

        // 两端都是一样, 醒来后再check退出条件.
        // p端只要生产的数量够了就不用在做了.
        if (producedCnt == totalItems) {
            // 当有一方开始退出的时候, 通知对方醒来
            // 否则一方全部退出了, 对方可能还有一些在睡觉, 干等(事实上已经没有东西了.)
            cons_cv.notify_all();
            PRINTF("producer%d exits\n", id);
            break;
        }

        // 自动拿到锁了, add item we produced
        // 注意, 上面sleep模拟item生产完了, 这里只是给生产的item一个名字,
        // 用producedCnt变量要在lock时候用, 不能在上面sleep后用.
        int item = producedCnt++;
        q.push(item);
        PRINTF("producer%d adding item id = %d, now queue size = %zu\n", id, item, q.size());

        lck.unlock();
        // item进queue了, queue肯定不是空, 叫consumer起来干活.
        cons_cv.notify_one();
    }
}

void consumer(int id) {
    PRINTF("consumer%d\n", id);
    while (true) {
        // consumer起来直接拿锁, 从q里拿东西.
        unique_lock<mutex> lck(mtx);
        cons_cv.wait(lck, [id] {
            // q is not emqpty, ready
            // or produced enough items => wake up to quit
            if (!q.empty() || producedCnt == totalItems) {
                return true;
            }
            PRINTF("q is empty, consumer%d sleep...\n", id);
            return false;
        });

        // c端要确认queue里面空了, 同时生产的数量够了.
        // 可能也可以用consumedCnt够了就退出, 但是不好, 如果有bug, q里面还有剩下的就leak了.
        // 还是把生产的都处理完, 然后再main里面check consumedCnt和生产的一样.
        if (q.empty() && producedCnt == totalItems) {
            prod_cv.notify_all();
            PRINTF("consumer%d exits\n", id);
            break;
        }

        // 自动拿到锁了, get item
        int item = q.front();
        q.pop();
        consumedCnt++;
        PRINTF("consumer%d getting item %d, now queue size = %zu\n", id, item, q.size());

        lck.unlock();
        prod_cv.notify_one();

        // similate for consuming item.
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int cppMain() {
    PRINTF("%s\n", __FUNCTION__);
    vector<thread> prod, cons;
    // test cases:
    // 3, 3, 两个一样多, q里面只有少量几个item
    // 9, 3, prod很多, queue马上就填满了
    // 3, 9, cons很多, queue几乎都是空的.
    const int prodNum = 3;
    const int consNum = 3;

    for (int i = 0; i < prodNum; ++i) {
        prod.emplace_back(producer, i);
    }
    for (int i = 0; i < consNum; ++i) {
        prod.emplace_back(consumer, i);
    }

    // 等待所有生产者结束
    for (auto& elem : prod) {
        elem.join();
    }

    // 等待所有消费者结束
    for (auto& elem : cons) {
        elem.join();
    }

    // cout << "q.empty()=" << q.empty()
    //     << ", producedCnt=" << producedCnt
    //     << ", consumedCnt" << consumedCnt
    //     << ", totalItems" << totalItems << endl;
    assert(q.empty() && producedCnt == totalItems && consumedCnt == totalItems);

    return 0;
}

}  // namespace thread_prod_cons

/*===== Output =====

[RUN  ] thread_prod_cons
[tid=1] cppMain
[tid=2] producer0
[tid=3] producer1
[tid=4] producer2
[tid=5] consumer0
[tid=5] q is empty, consumer0 sleep...
[tid=6] consumer1
[tid=6] q is empty, consumer1 sleep...
[tid=7] consumer2
[tid=7] q is empty, consumer2 sleep...
[tid=4] producer2 adding item id = 0, now queue size = 1
[tid=3] producer1 adding item id = 1, now queue size = 2
[tid=5] consumer0 getting item 0, now queue size = 1
[tid=6] consumer1 getting item 1, now queue size = 0
[tid=2] producer0 adding item id = 2, now queue size = 1
[tid=7] consumer2 getting item 2, now queue size = 0
[tid=2] producer0 adding item id = 3, now queue size = 1
[tid=6] consumer1 getting item 3, now queue size = 0
[tid=7] q is empty, consumer2 sleep...
[tid=3] producer1 adding item id = 4, now queue size = 1
[tid=4] producer2 adding item id = 5, now queue size = 2
[tid=5] consumer0 getting item 4, now queue size = 1
[tid=7] consumer2 getting item 5, now queue size = 0
[tid=7] q is empty, consumer2 sleep...
[tid=6] q is empty, consumer1 sleep...
[tid=4] producer2 adding item id = 6, now queue size = 1
[tid=5] consumer0 getting item 6, now queue size = 0
[tid=3] producer1 adding item id = 7, now queue size = 1
[tid=2] producer0 adding item id = 8, now queue size = 2
[tid=7] consumer2 getting item 7, now queue size = 1
[tid=6] consumer1 getting item 8, now queue size = 0
[tid=3] producer1 adding item id = 9, now queue size = 1
[tid=6] consumer1 getting item 9, now queue size = 0
[tid=7] q is empty, consumer2 sleep...
[tid=4] producer2 adding item id = 10, now queue size = 1
[tid=2] producer0 adding item id = 11, now queue size = 2
[tid=5] consumer0 getting item 10, now queue size = 1
[tid=7] consumer2 getting item 11, now queue size = 0
[tid=6] q is empty, consumer1 sleep...
[tid=7] q is empty, consumer2 sleep...
[tid=5] q is empty, consumer0 sleep...
[tid=4] producer2 adding item id = 12, now queue size = 1
[tid=2] producer0 adding item id = 13, now queue size = 2
[tid=3] producer1 adding item id = 14, now queue size = 3
[tid=6] consumer1 getting item 12, now queue size = 2
[tid=7] consumer2 getting item 13, now queue size = 1
[tid=5] consumer0 getting item 14, now queue size = 0
[tid=5] q is empty, consumer0 sleep...
[tid=7] q is empty, consumer2 sleep...
[tid=3] producer1 adding item id = 15, now queue size = 1
[tid=4] producer2 adding item id = 16, now queue size = 2
[tid=6] consumer1 getting item 15, now queue size = 1
[tid=7] consumer2 getting item 16, now queue size = 0
[tid=2] producer0 adding item id = 17, now queue size = 1
[tid=5] consumer0 getting item 17, now queue size = 0
[tid=3] producer1 adding item id = 18, now queue size = 1
[tid=6] consumer1 getting item 18, now queue size = 0
[tid=7] q is empty, consumer2 sleep...
[tid=2] producer0 adding item id = 19, now queue size = 1
[tid=4] producer2 adding item id = 20, now queue size = 2
[tid=5] consumer0 getting item 19, now queue size = 1
[tid=7] consumer2 getting item 20, now queue size = 0
[tid=7] q is empty, consumer2 sleep...
[tid=5] q is empty, consumer0 sleep...
[tid=3] producer1 adding item id = 21, now queue size = 1
[tid=4] producer2 adding item id = 22, now queue size = 2
[tid=6] consumer1 getting item 21, now queue size = 1
[tid=2] producer0 adding item id = 23, now queue size = 2
[tid=7] consumer2 getting item 22, now queue size = 1
[tid=5] consumer0 getting item 23, now queue size = 0
[tid=4] producer2 adding item id = 24, now queue size = 1
[tid=7] consumer2 getting item 24, now queue size = 0
[tid=5] q is empty, consumer0 sleep...
[tid=6] q is empty, consumer1 sleep...
[tid=3] producer1 adding item id = 25, now queue size = 1
[tid=2] producer0 adding item id = 26, now queue size = 2
[tid=5] consumer0 getting item 25, now queue size = 1
[tid=6] consumer1 getting item 26, now queue size = 0
[tid=6] q is empty, consumer1 sleep...
[tid=5] q is empty, consumer0 sleep...
[tid=2] producer0 adding item id = 27, now queue size = 1
[tid=6] consumer1 getting item 27, now queue size = 0
[tid=3] producer1 adding item id = 28, now queue size = 1
[tid=7] consumer2 getting item 28, now queue size = 0
[tid=5] q is empty, consumer0 sleep...
[tid=4] producer2 adding item id = 29, now queue size = 1
[tid=5] consumer0 getting item 29, now queue size = 0
[tid=4] producer2 exits
[tid=7] consumer2 exits
[tid=3] producer1 exits
[tid=5] consumer0 exits
[tid=6] consumer1 exits
[tid=2] producer0 exits
[Memory Report] allocationCnt = deallocationCnt = 10, totalAllocatedSize = totalDeallocatedSize = 704 Bytes
[   OK] thread_prod_cons

*/
