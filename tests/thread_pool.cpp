#include <condition_variable>
#include <functional>  // function
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
using namespace std;

#include "utils.h"

namespace thread_pool {

//=========================================================
// 实现 ThreadPool
// 线程的入口函数只能是无参, 且返回空, 任意参数的太烦了, 略.
/*
follow up:
1,
让线程池支持优先队列（高优先级任务先执行）, 数字小的优先级高
定义一个包装任务结构 PrioritizedTask，包含 priority 和 function<void()>.
struct PrioritizedTask {
    int priority;
    function<void()> task;
    bool operator<(const PrioritizedTask& other) const {
        // 小的优先级值代表更高优先级，priority_queue是大顶堆
        return priority > other.priority;
    }
};

用 std::priority_queue 替换原来的 queue<function<void()>>.
priority_queue<PrioritizedTask> tasks;

修改 submit() 支持传入优先级。
void submit(function<void()> task, int priority = 0) {
    ...
    tasks.push(PrioritizedTask{priority, move(task)});
    ...
}
修改工作线程中取任务的逻辑。
task = move(tasks.top().task); // 而不是front()

完全可以用 std::pair<int, function<void()>> 来替代 PrioritizedTask 结构体
priority_queue<
    pair<int, function<void()>>,
    vector<pair<int, function<void()>>>,
    greater<>
> tasks;
priority_queue 默认是 大顶堆（最大值先出队），但我们希望数值小的优先级更高（即优先执行），所以要用 greater<> 变成小顶堆。
std::pair<int, function<void()>> 会先按 int 比较，刚好适用于优先级调度。
*/

class ThreadPool {
private:
    queue<function<void()>> tasks;
    bool stop;
    mutex mtx;  // 保护上面两个
    condition_variable cv;
    vector<thread> workers;

public:
    ThreadPool(size_t cnt) : stop(false) {
        for (size_t i = 0; i < cnt; ++i) {
            workers.emplace_back([&] {
                while (true) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lck(mtx);
                        // 所有线程等着, 直到queue里面来活了.
                        cv.wait(lck, [&] { return stop || !tasks.empty(); });
                        // 不能单单只check stop, 多线程小心些.
                        // 在某些极端情况下（例如刚调用 stop=true 但队列中还有任务没被处理完），
                        // worker 线程可能提前退出，任务得不到执行！
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = move(tasks.front());
                        tasks.pop();
                    }
                    // 执行task不用lock, lock/mutex只保护成员变量
                    task();
                }
                // worker执行完了又重新休眠等待.
            });
        }
    }
    // 如果要加 shutdown()，让线程池完成当前任务后退出,
    // 就是把析构里面的放到shutdown里, 然后析构call shutdown.
    ~ThreadPool() {
        {
            unique_lock<mutex> lck(mtx);
            stop = true;
        }
        cv.notify_all();
        // 什么时候线程对象不是 joinable 的？
        // 默认构造的 std::thread t;（没有启动线程） → !t.joinable()
        // 线程被 join() 过了 → 不能再 join()
        // 线程被 detach() 了 → 不能 join()
        // 为什么在析构里很重要？
        // 因为如果线程池中的某个 thread 对象在程序其它地方（或异常路径）
        // 被提前 join() 或 detach() 了，joinable() 会变成 false。
        // 如果不加判断就直接调用 join()，程序会崩溃。
        // 加上 joinable() 是一种 防御性编程，即使你确定不会发生，也值得保留。
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
    }

    // 有mutex等, 不能copy
    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

    void submit(function<void()> task) {
        unique_lock<mutex> lck(mtx);
        if (stop) {
            PRINTF("Error, pool stopped.");
            return;
        }
        tasks.push(move(task));
        lck.unlock();
        cv.notify_one();
    }
};

void taskFn() {
    PRINTF("Task2.\n");
}
int cppMain() {
    ThreadPool pool(3);
    pool.submit([] { PRINTF("Task1.\n"); });
    pool.submit(taskFn);

    this_thread::sleep_for(chrono::milliseconds(100));

    return 0;
}

}  // namespace thread_pool

/*===== Output =====

[RUN  ] thread_pool
[tid=2] Task1.
[tid=2] Task2.
[   OK] thread_pool

*/
