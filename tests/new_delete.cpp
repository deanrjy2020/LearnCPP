#include <cassert>
#include <cstdint>  // uint8_t
#include <iostream>
#include <new>  // placement new
#include <vector>
using namespace std;

namespace new_delete {

//=========================================================
// 例子: 类内重载(overload) new / delete
// 例子: placement new
// 全局的new/delete重载在utils.cpp里实现, 用来检测整个项目的mem leak

class A {
    int x;
    int y;

public:
    A(int x) : x(x), y(x * x) {}
    ~A() {}
    void print() const { cout << "x=" << x << endl; }
};

class B {
    int x;

public:
    // 重载 operator new
    void* operator new(size_t size) {
        cout << "[B::new] allocating " << size << " bytes\n";
        return malloc(size);
    }

    // 重载 operator delete
    void operator delete(void* ptr) {
        cout << "[B::delete] freeing memory\n";
        free(ptr);
    }

    // 重载 operator new[]
    void* operator new[](size_t size) {
        cout << "[B::new[]] Requesting " << size << " bytes\n";
        void* ptr = malloc(size);
        return ptr;
    }

    // 重载 operator delete[]
    void operator delete[](void* ptr) {
        cout << "[B::delete[]] Freeing memory\n";
        free(ptr);
    }
};

void subtest1() {
    cout << __FUNCTION__ << endl;
    cout << "--- overload operator new 101 ---" << endl;
    {
        // 编译器生成的调用会变成：void* mem = B::operator new(sizeof(B));  // size_t = 4
        B* obj = new B();  // 会调用重载的 new
        delete obj;        // 会调用重载的 delete
        obj = nullptr;

        // 同理: void* mem = B::operator new(sizeof(B) * 3); // = 12
        B* arr = new B[3];  // 调用 operator new[]
        delete[] arr;       // 调用 operator delete[]
        arr = nullptr;
    }

    cout << "--- placement new 101 ---" << endl;
    {
        // A里面有两个int, a的大小是8, 如果buf<8, 编译出错, 空间不够给a.
        uint8_t buf[100];  // 100B
        A* a = new (buf) A(1);
        cout << "buf=" << (void*)buf << ", a=" << a
             << ", sizeof(A)=" << sizeof(A) << endl;
        assert((void*)buf == (void*)a);
        a->print();  // use a
        a->~A();     // 必须显式调用析构函数，否则不会自动清理资源
        a = nullptr;
    }
}

//=========================================================
// "重载operator new/delete + 内存池" 和 "placement new + 内存池"
// 内存池用的是同一个简单的内存池, 提前分配固定大小内存块，避免频繁 malloc/free
// 先看singleton.cpp里的例子, 再看这个
// followup: 可加入线程安全（用 std::mutex）；
//
// "重载operator new/delete + 内存池"
// 全局内存分配计数器, 可用于检测泄漏

// 简单的内存池, 用vector实现, 预先分配多个放到vector里面,
// 每个pool对象给固定大小的block用, 不同大小用不同的pool对象,
// 注意, 只是代码共用, 给MyClassC用的MemoryPool<MyClassC>和
// 给MyClassD用的MemoryPool<MyClassD>是两个pool.
//
// 模板T只是用它的sizeof(T), 还可以用memory pool manager包住, manager里面用unordered_map<size_t, MemoryPool*>
// 见memory_pool.cpp
template <typename T>
class MemoryPool {
    // blockSize就是要放的MyClassC类对象的大小, 见单例入口getMemoryPool()
    const size_t blockSize;
    // 里面放的都是还没有用的block空指针,
    // allocate的时候把空指针返回给caller(class的new operator), 然后app保存
    // deallocate的时候空指针传回来, 放回这个vector去
    vector<void*> freeBlocks;

public:
    // 单例池
    // 如果用模板, getMemoryPool可以在这里实现, 标准的singleton.
    // 如果是单单个一个特定的类MyClassC用, 函数里面要用到sizeof(MyClassC)而不是sizeof(T)
    // 但是MyClassC 还未完全定义，不能用 sizeof(MyClassC)
    // 就要延迟实现 getMemoryPool, 在MyClassC后实现
    static MemoryPool<T>& getMemoryPool() {
        // 使用 static 局部变量确保内存池只初始化一次
        cout << "[getMemoryPool] to return singleton instance." << endl;
        static MemoryPool<T> pool(sizeof(T), 5);  // pool里面可以放5个T(MyClassC)
        return pool;
    }

    MemoryPool(size_t blockSz, size_t blockNum = 10) : blockSize(blockSz) {
        freeBlocks.reserve(blockNum);
        for (size_t i = 0; i < blockNum; ++i) {
            freeBlocks.push_back(malloc(blockSize));
        }
    }

    ~MemoryPool() {
        for (auto ptr : freeBlocks) {
            free(ptr);
        }
    }

    void* allocate() {
        if (freeBlocks.empty()) {
            return malloc(blockSize);
        } else {
            void* ptr = freeBlocks.back();
            freeBlocks.pop_back();
            return ptr;
        }
    }

    void deallocate(void* ptr) {
        freeBlocks.push_back(ptr);
    }
};

// 全局内存使用计数
static size_t g_allocCnt = 0;
static size_t g_deallocCnt = 0;

// 类定义（此时 MyClassC 的完整定义已经可见）
class MyClassC {
    int data;

public:
    MyClassC() : data(42) {
        cout << "[MyClassC::constructor] constructed\n";
    }
    ~MyClassC() {
        cout << "[MyClassC::destructor] destroyed\n";
    }

    // 重载 new
    void* operator new(size_t size) {
        g_allocCnt++;
        void* ptr = MemoryPool<MyClassC>::getMemoryPool().allocate();
        cout << "[MyClassC::new] " << size << " bytes, g_allocCnt = " << g_allocCnt << "\n";
        return ptr;
    }

    // 重载 delete
    void operator delete(void* ptr) {
        g_deallocCnt++;
        MemoryPool<MyClassC>::getMemoryPool().deallocate(ptr);
        cout << "[MyClassC::delete] g_deallocCnt = " << g_deallocCnt << "\n";
    }
};

// 延迟实现 getMemoryPool例子
// 不是用模板T, 而是单单给MyClassC用
// 现在才可以实现 getMemoryPool，因为 MyClassC 类型已经完全定义了
// MemoryPool& MemoryPool::getMemoryPool() {
//     cout << "[getMemoryPool] to return singleton instance." << endl;
//     static MemoryPool pool(sizeof(MyClassC), 5);  // pool里面可以放5个MyClassC
//     return pool;
// }

// MyClassD和MyClassC是一样的.
// operator new 是静态分配阶段调用的函数，它在对象构造之前被调用，此时对象还没有虚函数表（vtable）。
// 虚函数需要依赖对象的 vtable 指针（通常存在对象内存的起始位置），但此时连对象内存都还没分配，根本无法访问 vtable。
class MyClassD {
    int data;

public:
    MyClassD() : data(43) {
        cout << "[MyClassD::constructor] constructed\n";
    }
    ~MyClassD() {
        cout << "[MyClassD::destructor] destroyed\n";
    }

    // 重载 new
    void* operator new(size_t size) {
        g_allocCnt++;
        void* ptr = MemoryPool<MyClassD>::getMemoryPool().allocate();
        cout << "[MyClassD::new] " << size << " bytes, g_allocCnt = " << g_allocCnt << "\n";
        return ptr;
    }

    // 重载 delete
    void operator delete(void* ptr) {
        g_deallocCnt++;
        MemoryPool<MyClassD>::getMemoryPool().deallocate(ptr);
        cout << "[MyClassD::delete] g_deallocCnt = " << g_deallocCnt << "\n";
    }
};

void subtest2() {
    cout << __FUNCTION__ << endl;
    std::cout << "--- overload operator new/delete + memory pool ---\n";
    MyClassC* c1 = new MyClassC();
    MyClassC* c2 = new MyClassC();
    MyClassD* d1 = new MyClassD();
    delete c1;
    delete c2;
    delete d1;
    c1 = nullptr;
    c2 = nullptr;
    d1 = nullptr;
    assert(g_allocCnt == g_deallocCnt);
    cout << "[Summary] allocs: " << g_allocCnt << ", deallocs: " << g_deallocCnt << "\n";

    std::cout << "--- replacement new + memory pool ---\n";
    MemoryPool<MyClassC>& pool = MemoryPool<MyClassC>::getMemoryPool();
    // 从内存池中获取原始内存
    void* raw1 = pool.allocate();
    void* raw2 = pool.allocate();
    // 在这块原始内存上构造对象（placement new）
    // 在 C++ 中，当在类中定义了自定义的 operator new(size_t) 时，
    // 这个定义会隐藏全局作用域中的所有 operator new 函数，
    // 包括标准的 placement new 版本 operator new(size_t, void*)。​
    // 因此，在类作用域内使用 placement new（例如 new(ptr) MyClassC();）时，
    // 编译器会首先查找类内的匹配函数，找不到后不会继续查找全局作用域中的版本，除非您显式地引入它。
    // 用::new显式调用全局版本
    MyClassC* obj1 = ::new (raw1) MyClassC();
    MyClassC* obj2 = ::new (raw2) MyClassC();
    // 使用完后手动析构对象（析构函数不会自动调用）
    obj1->~MyClassC();
    obj2->~MyClassC();
    // 手动归还内存池
    pool.deallocate(obj1);
    pool.deallocate(obj2);
    obj1 = nullptr;
    obj2 = nullptr;
}

int cppMain() {
    subtest1();
    subtest2();

    return 0;
}

}  // namespace new_delete

/*===== Output =====

[RUN  ] new_delete
subtest1
--- overload operator new 101 ---
[B::new] allocating 4 bytes
[B::delete] freeing memory
[B::new[]] Requesting 12 bytes
[B::delete[]] Freeing memory
--- placement new 101 ---
buf=0xf9c07ff9d0, a=0xf9c07ff9d0, sizeof(A)=8
x=1
subtest2
--- overload operator new/delete + memory pool ---
[getMemoryPool] to return singleton instance.
[MyClassC::new] 4 bytes, g_allocCnt = 1
[MyClassC::constructor] constructed
[getMemoryPool] to return singleton instance.
[MyClassC::new] 4 bytes, g_allocCnt = 2
[MyClassC::constructor] constructed
[getMemoryPool] to return singleton instance.
[MyClassD::new] 4 bytes, g_allocCnt = 3
[MyClassD::constructor] constructed
[MyClassC::destructor] destroyed
[getMemoryPool] to return singleton instance.
[MyClassC::delete] g_deallocCnt = 1
[MyClassC::destructor] destroyed
[getMemoryPool] to return singleton instance.
[MyClassC::delete] g_deallocCnt = 2
[MyClassD::destructor] destroyed
[getMemoryPool] to return singleton instance.
[MyClassD::delete] g_deallocCnt = 3
[Summary] allocs: 3, deallocs: 3
--- replacement new + memory pool ---
[getMemoryPool] to return singleton instance.
[MyClassC::constructor] constructed
[MyClassC::constructor] constructed
[MyClassC::destructor] destroyed
[MyClassC::destructor] destroyed
[   OK] new_delete

*/
