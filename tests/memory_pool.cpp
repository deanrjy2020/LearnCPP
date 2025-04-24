#include <cassert>
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
            // 如果不等, 可能有mem leak
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
ptr[0]=0xff6770
ptr[1]=0xff6760
ptr[2]=0xff6750
ptr[3]=0xff6fa0
~MemoryPool()
ptr[0] = 0xff6fa0
ptr[1] = 0xff6750
ptr[2] = 0xff6760
ptr[3] = 0xff6770
ptr[4] = 0xff6f90
ptr[5] = 0xff6f80
totalBlockNum = 6, blockNum = 3, chunks.size() = 2
chunk[0] addr = 0xff6750
chunk[1] addr = 0xff6f80
--- memory pool manager allocate and deallocate ---
ptr1 = 0xff67b0, ptr2 = 0xff67a8, ptr3 = 0x27b7150
~MemoryPool()
ptr[0] = 0x27b7150
ptr[1] = 0x27b7140
ptr[2] = 0x27b7130
ptr[3] = 0x27b7120
ptr[4] = 0x27b7110
totalBlockNum = 5, blockNum = 5, chunks.size() = 1
chunk[0] addr = 0x27b7110
~MemoryPool()
ptr[0] = 0xff67a8
ptr[1] = 0xff67b0
ptr[2] = 0xff67a0
ptr[3] = 0xff6798
ptr[4] = 0xff6790
totalBlockNum = 5, blockNum = 5, chunks.size() = 1
chunk[0] addr = 0xff6790
--- memory pool + placement new ---
x=3
~MemoryPool()
ptr[0] = 0xff6f58
ptr[1] = 0xff6f50
totalBlockNum = 2, blockNum = 2, chunks.size() = 1
chunk[0] addr = 0xff6f50
[   OK] memory_pool

*/
