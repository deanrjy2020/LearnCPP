#include <iostream>
#include <memory>  // sptr
using namespace std;

namespace destructor_basic {

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
    //friend class shared_ptr<MyClass>;
    //friend struct default_delete<MyClass>;
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
[   OK] destructor_basic

*/
