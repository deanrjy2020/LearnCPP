#include <cassert>
#include <iostream>
using namespace std;

namespace virtual_basic {

//===================================================
// virtual基础用法

class dog {
public:
    dog() {}
    virtual ~dog() {}  // 有virtual的都需要virtual des
    // 父类如果是virtual, 子类默认也是.
    virtual void bark() const {
        cout << "Woof, I am just a dog." << endl;
    }
    void seeCat() const { bark(); }
};

class yellowdog : public dog {
public:
    yellowdog() {}
    void bark() const override { cout << "Woof, I am a yellow dog." << endl; }
};

void subtest1() {
    cout << __FUNCTION__ << endl;
    cout << "Example 1, non-pointer." << endl;
    yellowdog yd;
    yd.bark();  // 和父类没关系, call子类自己的.
    // 没有virtual, call父类seeCat, 然后call父类的bark
    // 有virtual, call父类seeCat, 然后call子类的bark, runtime时动态选择
    yd.seeCat();

    cout << "Example 2, pointer." << endl;
    // 父类指针指向子类
    dog* d = new yellowdog();
    // 没有virtual, 不是多肽, 父类自己的bark, 即使是用子类new出来的, call的时候是父类指针.
    // 果virtual, 就是call子类的
    d->bark();
    // 没有virtual, 都在父类里面.
    // 有virtual, 父类的seeCat会call子类的bark
    d->seeCat();
}

//===================================================
// 抽象类（不能被实例化）, pure virtual example.

class Animal {
public:
    virtual ~Animal() {}

    // 纯虚函数：派生类必须实现
    virtual void speak() const = 0;

    // 可以有非纯虚函数
    void info() const {
        cout << "This is an animal.\n";
    }
};

class Dog : public Animal {
public:
    void speak() const override {
        cout << "Woof!\n";
    }
};

class Cat : public Animal {
public:
    void speak() const override {
        cout << "Meow~\n";
    }
};

void subtest2() {
    cout << __FUNCTION__ << endl;
    // 和上面一样, 也可以使用非指针, 下面是指针的例子.
    Animal* dog = new Dog();
    Animal* cat = new Cat();

    dog->speak();  // 输出: Woof!
    cat->speak();  // 输出: Meow~

    dog->info();  // 输出: This is an animal.
    cat->info();

    delete dog;
    delete cat;
}

//===================================================
// 虚函数表 virtual table
typedef void (*Func)(void);  // 函数指针

class Base {
public:
    Base() { cout << "Base::Base()" << endl; }
    // vtable都是放在实例的最开始的, 不管前面有member vari还是有其他函数foo()
    void foo1() {
        cout << "regular func Base::foo1()" << endl;
    }
    virtual void f() {  // func ptr 0
        cout << "Base::f()" << endl;
    }
    virtual void g() {  // func ptr 1
        cout << "Base::g()" << endl;
    }
    // 有virtual的都需要virtual des, 否则编译警告(如果caller有new).
    // 故意把destructor放在这里, 在vtable里面占用了2个指针, ptr2和ptr3
    // 在PrintVtable()里面skip掉了, 否则crash
    // 如果不写destructor, vtable里面就没有这个问题.
    // todo, 注意, 即使中间有析构函数, 从打印的函数地址看, f, g, h三个函数地址相差都是64(0x40)
    virtual ~Base() {
        cout << "Base::~Base()" << endl;
    }
    virtual void h() {  // func ptr 4
        cout << "Base::h()" << endl;
    }
};

class Derive : public Base {
public:
    Derive() { cout << "Derive::Derive()" << endl; }
    void foo2() {
        cout << "regular func Derive::foo2()" << endl;
    }
    virtual void f() override {  // func ptr 0, 覆盖父类
        cout << "Derive::f()" << endl;
        // 假设a是Base里面的protected, 这里用a也会影响func ptr, 可能就要多占用位置了.
        // cout << "Derive::f(), a = " << a << endl;
    }
    virtual void g1() {  // func ptr 5
        cout << "Derive::g1()" << endl;
    }
    // 这里不会占用ptr 6, 应该也是覆盖了父类? 不得而知.
    virtual ~Derive() {
        cout << "Derive::~Derive()" << endl;
    }
    virtual void h1() {  // func ptr 6
        cout << "Derive::h1()" << endl;
    }
};

// 只是给subtest3用的, 中间skip了一些, 其他test不要用.
void PrintVtable(void* pObj) {
    // The value in the first addr is the vtable address
    // 大多数实现上，虚函数表指针一般都放在对象第一个位置
    // vtable是一个二维数组, 即void**, 要取pObj里面的值即*pObj
    // 先把pObj转化成void***类型, 取值后为void**
    void** vtable = *(void***)pObj;

    // 输出虚函数表中每个函数的地址, 以nullptr结尾
    // 碰到nullptr了就不打, 目前okay, 反正都是debug看virtual table
    for (int i = 0; vtable[i] != nullptr; i++) {
        cout << "Func ptr " << i << ": " << vtable[i] << endl;
        // 将虚函数表中的虚函数转换为函数指针，然后进行调用
        Func func = (Func)vtable[i];
        if (i == 2 || i == 3) {
            cout << "skip calling func ptr." << endl;
        } else {
            func();
        }
    }
}

void subtest3() {
    cout << __FUNCTION__ << endl;

    cout << "=====Base vtable:" << endl;
    {
        Base b;
        // 打印父类的虚函数表
        PrintVtable(&b);
    }

    cout << "=====Derive Vetable:" << endl;
    {
        Derive d;
        // 打印子类的虚函数表
        PrintVtable(&d);
    }

    // 一般使用
    cout << "=====Polymorphism:" << endl;
    {
        Base* b = new Derive;
        b->f();
        b->foo1();
        delete b;
        b = nullptr;
    }
    cout << __FUNCTION__ << " ended" << endl;
}

//===================================================
// C++ 对象的内存布局
class Parent {
public:
    int iparent;
    Parent() : iparent(10) {}
    virtual void f() { cout << "Parent::f()" << endl; }
    virtual void g() { cout << "Parent::g()" << endl; }
    virtual void h() { cout << "Parent::h()" << endl; }
};

class Child : public Parent {
public:
    int ichild;
    Child() : ichild(100) {}
    virtual void f() override { cout << "Child::f()" << endl; }
    virtual void g_child() { cout << "Child::g_child()" << endl; }
    virtual void h_child() { cout << "Child::h_child()" << endl; }
};

class GrandChild : public Child {
public:
    int igrandchild;
    GrandChild() : igrandchild(1000) {}
    virtual void f() override { cout << "GrandChild::f()" << endl; }
    virtual void g_child() override { cout << "GrandChild::g_child()" << endl; }
    virtual void h_grandchild() { cout << "GrandChild::h_grandchild()" << endl; }
};

void subtest4() {
    cout << __FUNCTION__ << endl;

    cout << "=====GrandChild vtable:" << endl;
    {
        // both 8 bytes since it is 64-bit machine.
        // int size还是4, 不管在32位还是64位, 一般都是4 bytes
        cout << "sizeof(void*) = " << sizeof(void*)            // 8
             << ", sizeof(int*) = " << sizeof(int*)            // 8
             << ", sizeof(uintptr_t) = " << sizeof(uintptr_t)  // 8
             << ", sizeof(int) = " << sizeof(int)              // 4
             << endl;
        GrandChild gc;

        // 打印孙子类的虚函数表
        void** vtable = *(void***)&gc;
        cout << "vtable addr = " << vtable << endl;
        for (int i = 0; vtable[i] != nullptr; i++) {
            cout << "Func ptr " << i << ": " << vtable[i] << endl;
            Func func = (Func)vtable[i];
            func();
        }

        // 打印vtable后面的成员变量
        const int pointerSize = sizeof(void*);
        assert(pointerSize == 8);
        const int intSize = sizeof(int);
        assert(intSize == 4);
        // 这里不用uintptr_t, 用char*也可以.
        uintptr_t p = (uintptr_t)&gc;  // p是一个地址
        // 现在p上的值是vtable的地址, 还是地址, 所以用void**, 取一次地址*后还是void*类型.
        cout << "p = 0x" << hex << p << dec << ", *p = " << *(void**)p << endl;
        p += pointerSize;
        // 现在p上的值是iparent
        cout << "p = 0x" << hex << p << dec << ", *p = " << *(int*)p << endl;
        p += intSize;
        // 现在p上的值是ichild
        cout << "p = 0x" << hex << p << dec << ", *p = " << *(int*)p << endl;
        p += intSize;
        // 现在p上的值是igrandchild
        cout << "p = 0x" << hex << p << dec << ", *p = " << *(int*)p << endl;
        // 后面的就不管了...

        // 通过gc.xx打印的成员变量地址, 对比.
        cout << "Object address:      " << &gc << endl;
        cout << "vptr (vtable addr):  " << *(void**)&gc << endl;
        cout << "iparent address:     " << (void*)(&gc.iparent) << ", gc.iparent = " << gc.iparent << endl;
        cout << "ichild address:      " << (void*)(&gc.ichild) << ", gc.ichild = " << gc.ichild << endl;
        cout << "igrandchild address: " << (void*)(&gc.igrandchild) << ", gc.igrandchild = " << gc.igrandchild << endl;
    }

    cout << __FUNCTION__ << " ended" << endl;
}

int cppMain() {
    subtest1();
    subtest2();
    subtest3();
    subtest4();

    return 0;
}

}  // namespace virtual_basic

/*===== Output =====

[RUN  ] virtual_basic
subtest1
Example 1, non-pointer.
Woof, I am a yellow dog.
Woof, I am a yellow dog.
Example 2, pointer.
Woof, I am a yellow dog.
Woof, I am a yellow dog.
subtest2
Woof!
Meow~
This is an animal.
This is an animal.
subtest3
=====Base vtable:
Base::Base()
Func ptr 0: 0x7ff6b9685750
Base::f()
Func ptr 1: 0x7ff6b9685790
Base::g()
Func ptr 2: 0x7ff6b9685920
skip calling func ptr.
Func ptr 3: 0x7ff6b96858f0
skip calling func ptr.
Func ptr 4: 0x7ff6b96857d0
Base::h()
Base::~Base()
=====Derive Vetable:
Base::Base()
Derive::Derive()
Func ptr 0: 0x7ff6b9685b00
Derive::f()
Func ptr 1: 0x7ff6b9685790
Base::g()
Func ptr 2: 0x7ff6b9685c70
skip calling func ptr.
Func ptr 3: 0x7ff6b9685c40
skip calling func ptr.
Func ptr 4: 0x7ff6b96857d0
Base::h()
Func ptr 5: 0x7ff6b9685b40
Derive::g1()
Func ptr 6: 0x7ff6b9685b80
Derive::h1()
Derive::~Derive()
Base::~Base()
=====Polymorphism:
Base::Base()
Derive::Derive()
Derive::f()
regular func Base::foo1()
Derive::~Derive()
Base::~Base()
subtest3 ended
subtest4
=====GrandChild vtable:
sizeof(void*) = 8, sizeof(int*) = 8, sizeof(uintptr_t) = 8, sizeof(int) = 4
vtable addr = 0x7ff6b96ac730
Func ptr 0: 0x7ff6b9685550
GrandChild::f()
Func ptr 1: 0x7ff6b9685d10
Parent::g()
Func ptr 2: 0x7ff6b9685d50
Parent::h()
Func ptr 3: 0x7ff6b9685590
GrandChild::g_child()
Func ptr 4: 0x7ff6b9685a40
Child::h_child()
Func ptr 5: 0x7ff6b9685510
GrandChild::h_grandchild()
p = 0x7a167ff440, *p = 0x7ff6b96ac730
p = 0x7a167ff448, *p = 10
p = 0x7a167ff44c, *p = 100
p = 0x7a167ff450, *p = 1000
Object address:      0x7a167ff440
vptr (vtable addr):  0x7ff6b96ac730
iparent address:     0x7a167ff448, gc.iparent = 10
ichild address:      0x7a167ff44c, gc.ichild = 100
igrandchild address: 0x7a167ff450, gc.igrandchild = 1000
subtest4 ended
[   OK] virtual_basic

*/
