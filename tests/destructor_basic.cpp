#include <iostream>
#include <memory>  // sptr
using namespace std;

namespace destructor_basic {

//=========================================================
// private destructor用法1, 实现只能在heap上创建对象
// 适用于一些stack memory小的embedded system, 禁止栈上创建对象，只允许堆上创建 + 手动释放.
// follow up:
// 只能在栈上创建对象的类, 要实现这一点，我们可以将new操作符重载设为私有。这样就无法使用new来在堆上创建对象了。
// 在这个例子中，我们不能在堆上创建OnlyStack类的对象，因为new操作符已被私有化，但我们仍然可以在栈上创建。
// class OnlyStack {
// private:
//     void* operator new(size_t size) = delete;  // 禁用new操作符
//     void operator delete(void* p) = delete;  // 禁用delete操作符
//
// public:
//     OnlyStack() {}
//     ~OnlyStack() {}
// };
//
//
// 用法2, 见singleton

class OnlyHeap {
private:
    OnlyHeap() {
        cout << "Constructor called\n";
    }
    ~OnlyHeap() {
        cout << "Destructor called\n";
    }

public:
    static OnlyHeap* create() {
        return new OnlyHeap();
    }

    void destroy() {
        delete this;
    }
};

//=========================================================
// private destructor用法3, 实现只能在heap上创建对象
// Reference counting smart pointer. shared_ptr的实现内部拥有对私有析构函数的访问权限
// (因为它构造时就在类内创建), 所以不会报错；外部代码 不能直接 delete 或 new 这个类；
// 非常适合封装资源生命周期，比如图形资源、句柄等；

class MyClass {
public:
    static shared_ptr<MyClass> create() {
        // shared_ptr仍然可以访问private析构函数, 需要用自定义deleter，free p
        return shared_ptr<MyClass>(new MyClass(), [](MyClass* p) {
            cout << "my deleter, freeing p." << endl;
            delete p;  // 这里可以访问析构函数，因为是 friend
        });
    }

    void sayHello() {
        cout << "Hello from MyClass\n";
    }

private:
    MyClass() {
        cout << "Constructor called\n";
    }

    ~MyClass() {
        // todo, who called this?
        cout << "Destructor called\n";
    }

    // 禁止拷贝构造和赋值操作
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;

    // 禁止移动构造和赋值操作
    MyClass(MyClass&&) = delete;
    MyClass& operator=(MyClass&&) = delete;

    // 不知道是干什么的 todo
    // Declare shared_ptr as a friend to allow it to call the private destructor
    // friend class shared_ptr<MyClass>;
    // friend struct default_delete<MyClass>;
};

int cppMain() {
    {
        cout << "Subtest 1" << endl;
        // OnlyHeap obj;              // 错误：析构函数是 private，不能栈分配
        OnlyHeap* obj = OnlyHeap::create();  // ok
        obj->destroy();                      // ok
    }

    {
        cout << "Subtest 2" << endl;
        auto obj = MyClass::create();
        obj->sayHello();
        // 自动调用析构函数，无需手动 delete obj
    }

    return 0;
}

}  // namespace destructor_basic

/*===== Output =====

[RUN  ] destructor_basic
Subtest 1
Constructor called
Destructor called
Subtest 2
Constructor called
Hello from MyClass
my deleter, freeing p.
Destructor called
[tid=1] [Memory Report] globalNewCnt = 3, globalDeleteCnt = 3, globalNewMemSize = 158, globalDeleteMemSize = 158
[   OK] destructor_basic

*/
