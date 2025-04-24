#include <iostream>
using namespace std;

namespace inheritance_basic {

//===================================================
// 基础用法
class Base {
private:
    int a;

protected:
    int b;

public:
    Base(int a, int b) : a(a), b(b) { cout << "Base::Base()" << endl; }
    int getA() const { return a; }
    virtual void print() const {
        cout << "Base::print(), a = " << a << ", b = " << b << endl;
    }
    virtual ~Base() { cout << "Base::~Base(), a = " << a << ", b = " << b << endl; }
};

// public继承用的最多, 即父类里面的属性不变.
class Derive : public Base {
public:
    // 父类没有默认构造函数, 必须在子类构造函数里初始化父类, 否则会报错
    // 1, 先父类构造函数, 2, init list, 3, 自己的function body
    Derive(int a, int b) : Base(a, b) {
        cout << "Derive::Derive()" << endl;
        this->b = b + 1;
    }
    virtual void print() const override {  // 多肽.
        // 无法直接访问private a, 用getA()
        cout << "Derive::print(), a = " << getA() << ", b = " << b << endl;
    }
    // 和构造正好相反, 先析构自己, 再析构父类
    virtual ~Derive() { cout << "Derive::~Derive()" << endl; }
};

void subtest1() {
    cout << __FUNCTION__ << endl;
    Base* b = new Derive(1, 2);
    b->print();
    delete b;
    b = nullptr;
}

//===================================================
// 面试题
class A {
public:
    A() {
        printf("cons A\n");
        init();
    }
    virtual void init() { printf("init A\n"); }
};
class B : public A {
public:
    B() { printf("cons B\n"); }
    void init() override {
        A::init();
        printf("init B\n");
    }
};

void subtest2() {
    cout << __FUNCTION__ << endl;

    printf("1\n");
    // cons A -> init A -> cons B
    B b;
    printf("2\n");
    // print nothing
    A* a = &b;
    printf("3\n");
    // init A -> init B
    a->init();
}

int cppMain() {
    subtest1();
    subtest2();

    return 0;
}

}  // namespace inheritance_basic

/*===== Output =====

[RUN  ] inheritance_basic
subtest1
Base::Base()
Derive::Derive()
Derive::print(), a = 1, b = 3
Derive::~Derive()
Base::~Base(), a = 1, b = 3
subtest2
1
cons A
init A
cons B
2
3
init A
init B
[tid=1] [Memory Report] globalNewCnt = 1, globalDeleteCnt = 1, globalNewMemSize = 60, globalDeleteMemSize = 60
[   OK] inheritance_basic

*/
