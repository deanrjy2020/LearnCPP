#include <cassert>
#include <cstdint>  // uint8_t
#include <iostream>
#include <unordered_map>
#include <vector>
using namespace std;

namespace memory_pool {

//=========================================================
// linked list实现mem pool
// 构造的时候单块大小都是一样的, 块数传进来, 一次性分配连续的 blockSize*blockNum的内存
// 当前block的起始位置设置为指向下一个free块, 一直到nullptr.
// follow-up:
// 线程安全, 思路:
//     内存分配（allocate）：当多个线程同时请求内存时，需要防止竞争条件，确保每个线程分配到独立的内存块。
//     内存释放（deallocate）：当多个线程同时释放内存时，需要防止不同线程修改 freeList，从而导致数据竞争。
//     动态扩容（addChunk）：当内存池中的空闲内存块不够时，可能会进行扩容，这涉及到修改 freeList 和 chunks，这也是需要保护的地方。
//     实现, 已经在MemoryPool里实现, 注释掉, 用一个mutex
//     manager里面理论上也要加一个mutex保护那个map.

/*

另一道算法题, 和mem pool有关.
题目描述：实现简易内存分配器
实现一个简单的内存管理系统，它能对一块固定大小的连续内存空间进行分配与释放。
实现以下三个接口函数：
bool allocate(int pid, int size);       // 分配一段大小为 size 的内存给 pid
pair<int, int> get(int pid);            // 查询 pid 占用的内存区间
bool deallocate(int pid);               // 释放 pid 占用的内存

 背景说明：
你有一块固定大小的内存空间（假设大小为 N），可以视为一维数组 [0, N-1]。
所有内存从一开始都是空闲的，可以分配给任意 pid。
每次 allocate(pid, size) 会尝试为该 pid 分配一块长度为 size 的连续空闲空间（例如 first-fit 策略）。
每个 pid 最多只能分配一次；重复分配会失败，必须先 deallocate。
内存块分配成功后，应记录这个 pid 使用的区间，用于之后的查询和释放。

参数说明：
pid：一个整数，表示进程或线程 ID。可以认为是每个请求分配的“任务编号”。它不需要是系统真实的线程 ID，只是一个唯一编号（如 1、2、3...）。
size：请求分配的内存大小，单位为字节（或你定义的最小单位）。

例子:
allocate(1, 10);  // pid 1 -> 分配 [0,10)
allocate(2, 20);  // pid 2 -> 分配 [10,30)
get(1);           // 返回 {0,10}
deallocate(1);    // 释放 [0,10)
allocate(3, 5);   // pid 3 -> 分配 [0,5)（first-fit）

要求/挑战点：
要管理好空闲区间，支持合并相邻空闲块。
要快速查找是否有满足条件的空闲空间（可用 set / list 维护）。
注意边界条件（例如：释放未分配的 pid，不重复分配等）。

实现:
#include <iostream>
#include <map>
#include <set>
#include <vector>

using namespace std;

class MemoryAllocator {
    using Interval = pair<int, int>; // [start, end)
    int total_size;
    set<Interval> free_list;         // 空閒區間（按 start 排序）
    map<int, Interval> pid_map;      // pid -> 區間

public:
    MemoryAllocator(int size) : total_size(size) {
        free_list.insert({0, size});
    }

    bool allocate(int pid, int size) {
        if (pid_map.count(pid)) return false; // 已存在

        for (auto it = free_list.begin(); it != free_list.end(); ++it) {
            int start = it->first, end = it->second;
            if (end - start >= size) {
                int alloc_start = start;
                int alloc_end = start + size;
                pid_map[pid] = {alloc_start, alloc_end};

                // 更新 free_list
                free_list.erase(it);
                if (alloc_end < end) {
                    free_list.insert({alloc_end, end});
                }
                if (start < alloc_start) {
                    free_list.insert({start, alloc_start});
                }
                return true;
            }
        }
        return false; // 沒有足夠大的空閒空間
    }

    Interval get(int pid) {
        if (pid_map.count(pid)) return pid_map[pid];
        return {-1, -1}; // 未找到
    }

    bool deallocate(int pid) {
        if (!pid_map.count(pid)) return false;
        Interval freed = pid_map[pid];
        pid_map.erase(pid);

        // 合併空閒區間
        // 在set里面找到自己cur, 看前面有没有itv,
        // 有的话是否能连上, 能连上就把前面的prev去掉, 加到自己 (prev+cur)
        auto it = free_list.lower_bound(freed);
        if (it != free_list.begin()) {
            auto prev = prev(it);
            if (prev->second == freed.first) {
                // 前一段接得上
                freed.first = prev->first;
                free_list.erase(prev);
            }
        }
        
        // 这里要再找一遍, 2种情况
        // 1, 和前面连过了, 这时候已经是prev+cur, 找到的肯定是下一个next(包括end), 看能不能连上
        // 2, 和前面没连过, 这时候找到的还是自己cur, next=next(it), 看能不能连上.
        // 下面的代码好像有问题, 以后在测试. 总体思路没问题.
        it = free_list.lower_bound(freed);
        if (it != free_list.end() && it->first == freed.second) {
            // 後一段接得上
            freed.second = it->second;
            free_list.erase(it);
        }
        free_list.insert(freed);
        return true;
    }

    void print_state() {
        cout << "Free list:\n";
        for (auto& p : free_list) {
            cout << "[" << p.first << ", " << p.second << ")\n";
        }
        cout << "Allocated:\n";
        for (auto& [pid, range] : pid_map) {
            cout << "pid=" << pid << " => [" << range.first << ", " << range.second << ")\n";
        }
        cout << endl;
    }
};

int main() {
    MemoryAllocator mem(100);

    mem.allocate(1, 10);
    mem.allocate(2, 20);
    mem.print_state();

    mem.deallocate(1);
    mem.print_state();

    mem.allocate(3, 15);
    mem.print_state();

    auto p = mem.get(2);
    cout << "PID 2: [" << p.first << ", " << p.second << ")\n";

    return 0;
}







*/

// 另外一种实现方法是用vector, 见new_delete.cpp
class MemoryPool {
private:
    // chunk是一整大块, 里面有小块block
    vector<void*> chunks;
    void* freeList;
    size_t blockSize;
    // block num in one chunk, totalBlockNum = blockNum * chunks.size()
    size_t blockNum;
    // mutex mtx;  // 用于保护内存池操作, 即保护chunks和freeList

    void addChunk() {
        // lock_guard<mutex> lock(mtx);  // 保护扩容操作
        uint8_t* chunk = new uint8_t[blockSize * blockNum];
        chunks.push_back(chunk);

        // 构造空闲链表
        // 内存头是链表尾巴, 内存尾是链表头. 内存内容就是下一个node的地址.
        for (size_t i = 0; i < blockNum; ++i) {
            void* cur = chunk + i * blockSize;
            // 将当前chunk的起始位置设置为指向下一个free块
            *(void**)cur = freeList;
            freeList = cur;
        }
    }

public:
    MemoryPool(size_t blockSz, size_t blocks)
        : freeList(nullptr), blockSize(blockSz), blockNum(blocks) {
        // 确保每个block的大小能放的下指针(64bit为8B)
        assert(blockSize >= sizeof(void*));

        addChunk();
    }
    ~MemoryPool() {
        cout << "~MemoryPool()\n";
        // 测试code
        {
            // 过一遍linked list, 数一下block个数
            size_t id = 0;
            auto cur = freeList;
            while (cur != nullptr) {
                cout << "ptr[" << id++ << "] = " << cur << endl;
                cur = *(void**)cur;
            }
            // 如果不等, 说明当前pool析构的时候, pool里面还有block在用, 不算leak, 一般会给user一个warning.
            cout << "totalBlockNum = " << id << ", blockNum = " << blockNum << ", chunks.size() = " << chunks.size() << endl;
            assert(id == blockNum * chunks.size());
        }

        int chunkId = 0;
        for (auto& chunk : chunks) {
            cout << "chunk[" << chunkId++ << "] addr = " << chunk << endl;
            delete[] (uint8_t*)chunk;
            chunk = nullptr;
        }
    }

    void* allocate() {
        // lock_guard<mutex> lock(mtx);  // 保护分配操作
        if (freeList == nullptr) {
            addChunk();
        }
        auto res = freeList;
        freeList = *(void**)freeList;
        return res;
    }
    // add back to linked list.
    void deallocate(void* ptr) {
        // lock_guard<mutex> lock(mtx);  // 保护分配操作
        // 检查这个ptr是在某一个chunk base地址和chunk end地址之间,
        // 否则用户传进了一个不属于当前pool的地址, 导致未定义行为.
        bool isValidPtr = false;
        for (auto& chunk : chunks) {
            uint8_t* chunkStart = (uint8_t*)chunk;
            uint8_t* chunkEnd = chunkStart + blockSize * blockNum;
            if (ptr >= chunkStart && ptr < chunkEnd) {
                isValidPtr = true;
            }
        }
        if (!isValidPtr) {
            return;
        }

        *(void**)ptr = freeList;
        freeList = ptr;
    }
};

class MemoryPoolManager {
private:
    size_t alignUp(size_t n, size_t align) {
        // return (n + align - 1) / align * align;
        // 标准的align up的做法, 比上面好.
        return (n + align - 1) & ~(align - 1);
    }

    // <每个block的大小, MemoryPool>
    unordered_map<size_t, MemoryPool*> m_pools;
    // 每个MemoryPool里面初始化的block个数
    size_t blockNum;
    size_t alignment;

public:
    MemoryPoolManager(size_t blocks = 5, size_t align = 8)
        : blockNum(blocks), alignment(align) {}

    ~MemoryPoolManager() {
        for (auto& [size, pool] : m_pools) {
            delete pool;
        }
    }

    // 要开辟mem的block大小当做参数传进来.
    void* allocate(size_t blockSz) {
        size_t alignedBlockSz = alignUp(blockSz, alignment);
        if (m_pools.find(alignedBlockSz) == m_pools.end()) {
            m_pools[alignedBlockSz] = new MemoryPool(alignedBlockSz, blockNum);
        }
        return m_pools[alignedBlockSz]->allocate();
    }

    // 除了指针, block的大小也要, 对用户来讲
    void deallocate(void* ptr, size_t blockSz) {
        size_t alignedBlockSz = alignUp(blockSz, alignment);
        auto it = m_pools.find(alignedBlockSz);
        assert(it != m_pools.end());  // 防止错误释放
        it->second->deallocate(ptr);
    }
};

class A {
    // 稍微大一点, >=8B, mempool实现决定的, 要放指针.
    int x;
    int y;

public:
    A(int x) : x(x), y(x * x) {}
    ~A() {}
    void print() const { cout << "x=" << x << endl; }
};

void subtest1() {
    cout << __FUNCTION__ << endl;
    cout << "--- memory pool allocate and deallocate ---" << endl;
    {
        size_t blockSz = 16;  // 16B
        size_t blockNum = 4;
        MemoryPool pool(blockSz, blockNum - 1);  // 创建一个大小为 16B * 3 的内存池
        // 分配几个内存块
        vector<void*> ptrs(blockNum, nullptr);
        for (size_t i = 0; i < blockNum; ++i) {
            // 最后一个allocate出发扩容
            ptrs[i] = pool.allocate();
            cout << "ptr[" << i << "]=" << ptrs[i] << endl;
        }
        for (size_t i = 0; i < blockNum; ++i) {
            // 释放内存块
            pool.deallocate(ptrs[i]);
        }
    }

    cout << "--- memory pool manager allocate and deallocate ---" << endl;
    {
        MemoryPoolManager manager;
        void* ptr1 = manager.allocate(1);  // need 1B mem, XD
        void* ptr2 = manager.allocate(8);
        void* ptr3 = manager.allocate(10);
        cout << "ptr1 = " << ptr1 << ", ptr2 = " << ptr2 << ", ptr3 = " << ptr3 << endl;
        manager.deallocate(ptr1, 1);
        manager.deallocate(ptr2, 8);
        manager.deallocate(ptr3, 10);
    }

    // 更多的placement new见new_delete部分
    cout << "--- memory pool + placement new ---" << endl;
    {
        MemoryPool pool(sizeof(A), 2);
        void* raw = pool.allocate();
        A* a = new (raw) A(3);
        a->print();
        a->~A();
        pool.deallocate(raw);
        a = nullptr;
    }
}

int cppMain() {
    subtest1();

    return 0;
}

}  // namespace memory_pool

/*===== Output =====

[RUN  ] memory_pool
subtest1
--- memory pool allocate and deallocate ---
ptr[0]=0x1e7b09b6198
ptr[1]=0x1e7b09b6188
ptr[2]=0x1e7b09b6178
ptr[3]=0x1e7b09b64a8
~MemoryPool()
ptr[0] = 0x1e7b09b64a8
ptr[1] = 0x1e7b09b6178
ptr[2] = 0x1e7b09b6188
ptr[3] = 0x1e7b09b6198
ptr[4] = 0x1e7b09b6498
ptr[5] = 0x1e7b09b6488
totalBlockNum = 6, blockNum = 3, chunks.size() = 2
chunk[0] addr = 0x1e7b09b6178
chunk[1] addr = 0x1e7b09b6488
--- memory pool manager allocate and deallocate ---
ptr1 = 0x1e7b09971f8, ptr2 = 0x1e7b09971f0, ptr3 = 0x1e7b09973a8
~MemoryPool()
ptr[0] = 0x1e7b09973a8
ptr[1] = 0x1e7b0997398
ptr[2] = 0x1e7b0997388
ptr[3] = 0x1e7b0997378
ptr[4] = 0x1e7b0997368
totalBlockNum = 5, blockNum = 5, chunks.size() = 1
chunk[0] addr = 0x1e7b0997368
~MemoryPool()
ptr[0] = 0x1e7b09971f0
ptr[1] = 0x1e7b09971f8
ptr[2] = 0x1e7b09971e8
ptr[3] = 0x1e7b09971e0
ptr[4] = 0x1e7b09971d8
totalBlockNum = 5, blockNum = 5, chunks.size() = 1
chunk[0] addr = 0x1e7b09971d8
--- memory pool + placement new ---
x=3
~MemoryPool()
ptr[0] = 0x1e7b09971e0
ptr[1] = 0x1e7b09971d8
totalBlockNum = 2, blockNum = 2, chunks.size() = 1
chunk[0] addr = 0x1e7b09971d8
[tid=1] [Memory Report] globalNewCnt = 16, globalDeleteCnt = 16, globalNewMemSize = 1264, globalDeleteMemSize = 1264
[   OK] memory_pool

*/
