#include <cassert>
#include <functional>
#include <iostream>
#include <memory>  // unique_ptr
#include <mutex>
#include <stack>
using namespace std;

namespace object_pool {

#define FULL_MODE 1
//=========================================================
// 一个简单的object pool
// 满了自动double扩容
// 线程安全就是用一个mutex保护poolStack

template <typename T>
class ObjectPool {
public:
    // 下面的构造已经覆盖了这个, 有的话ObjectPool单个参数还是会用这个,
    // 去掉了都会用下面, 放着展示这个才是要的, 下面的不要求.
    ObjectPool(size_t capa) : capacity(0) {
        cout << "ObjectPool constructor 1 argu\n";
        unique_lock<mutex> lck(mtx);
        expandPool(capa);
    }

#if FULL_MODE
    // 接收任意构造参数，并保存构造工厂
    // 知道有这样的做法就好, 面试估计不会到这一步. 最多followup.
    template <typename... Args>
    ObjectPool(size_t capa, Args&&... args) : capacity(0) {
        cout << "ObjectPool constructor multiple argu\n";
        factory = [=]() { return make_unique<T>(args...); };

        unique_lock<mutex> lck(mtx);
        expandPool(capa);
    }
#endif

    ~ObjectPool() {}

    // 编译器在返回时会自动做 move
    // caller 拿到的是这个对象的唯一所有权
    unique_ptr<T> acquire() {
        unique_lock<mutex> lck(mtx);
        if (poolStack.empty()) {
            // double the pool capacity
            expandPool(capacity);
        }
        auto res = move(poolStack.top());
        poolStack.pop();
        return res;
    }

    void release(unique_ptr<T> obj) {
        unique_lock<mutex> lck(mtx);
        poolStack.push(move(obj));
    }

    // 获取池中剩余的对象数量
    size_t size() const {
        unique_lock<mutex> lck(mtx);
        return poolStack.size();
    }

private:
    void expandPool(size_t addSize) {
        // the mutex has been locked during this func.
        assert(poolStack.empty());

        for (size_t i = 0; i < addSize; ++i) {
            // 实际中这里只要选一个就可以, 不用共存.
            if (factory)
                // 任意参数版本
                poolStack.push(factory());
            else
                // 只能无参构造, T类里面要有无参的构造函数.
                poolStack.push(make_unique<T>());
        }

        this->capacity += addSize;
    }

    // mutable是为了在const函数中加锁.
    mutable mutex mtx;  // 保护poolStack和capa
    size_t capacity;
    // stack里面也可以放裸指针, 析构时候自己free mem.
    stack<unique_ptr<T>> poolStack;

    function<unique_ptr<T>()> factory;  // 保存构造函数
};

// 示例类，用于演示对象池的使用
class MyClass {
private:
    int a;
    string b;

public:
    // 这里必须要有无参构造, 否则expandPool里面make_unique的时候要传参数.
    MyClass() : a(0), b("none") {
        cout << "MyClass default constructor\n";
    }
    MyClass(int aa, string bb) : a(aa), b(bb) {
        cout << "MyClass overloaded constructor\n";
    }

    ~MyClass() {
        cout << "MyClass destructor\n";
    }

    void say_hello() {
        cout << "Hello from MyClass! a = " << a << ", b = " << b << endl;
    }
};

void subtest1() {
    cout << __FUNCTION__ << endl;
    {
        cout << "=====easy mode testing.\n";
        // 创建一个对象池，池中预分配 3 个 MyClass 对象
        ObjectPool<MyClass> pool(2);

        // 获取对象并使用
        auto obj1 = pool.acquire();
        obj1->say_hello();
        assert(pool.size() == 1);
        auto obj2 = pool.acquire();
        // trigger expand func
        auto obj3 = pool.acquire();
        assert(pool.size() == 1);
        // 归还对象
        pool.release(move(obj1));
        pool.release(move(obj2));
        assert(pool.size() == 3);
        // 特地不归还3
    }

#if FULL_MODE
    {
        cout << "=====full mode testing.\n";
        ObjectPool<MyClass> pool(2, 1, "apple");
        auto obj1 = pool.acquire();
        obj1->say_hello();
    }
#endif
}

int cppMain() {
    subtest1();

    cout << "cppMain done." << endl;
    return 0;
}

}  // namespace object_pool

/*===== Output =====

[RUN  ] object_pool
subtest1
=====easy mode testing.
ObjectPool constructor 1 argu
MyClass default constructor
MyClass default constructor
Hello from MyClass! a = 0, b = none
MyClass default constructor
MyClass default constructor
MyClass destructor
MyClass destructor
MyClass destructor
MyClass destructor
=====full mode testing.
ObjectPool constructor multiple argu
MyClass overloaded constructor
MyClass overloaded constructor
Hello from MyClass! a = 1, b = apple
MyClass destructor
MyClass destructor
cppMain done.
[tid=1] [Memory Report] globalNewCnt = 10, globalDeleteCnt = 10, globalNewMemSize = 1832, globalDeleteMemSize = 1832
[   OK] object_pool

*/
