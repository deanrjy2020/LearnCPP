#include <iostream>
using namespace std;

namespace template_basic {

//=========================================================
// template 101: 模板函数和模板类

template <typename T>
T add(T a, T b) {
    return a + b;
}

template <typename T1, typename T2, int extra, int defaultExtra = 10>
class A {
private:
    T1 a;
    T2 b;

public:
    A(T1 a, T2 b);
    T2 add();
};

template <typename T1, typename T2, int extra, int defaultExtra>
A<T1, T2, extra, defaultExtra>::A(T1 a, T2 b)
    : a(a), b(b) {
    cout << "extra = " << extra << ", defaultExtra = " << defaultExtra << endl;
}

template <typename T1, typename T2, int extra, int defaultExtra>
T2 A<T1, T2, extra, defaultExtra>::add() {
    return a + b + extra + defaultExtra;
}

void subtest1() {
    cout << __FUNCTION__ << endl;
    cout << add<int>(1, 2)
         << ", " << add<float>(1.1f, 2.2f)
         << ", " << add(string("Hello, "), string("World!"))
         << endl;

    A<int, float, 5> a(1, 2.2);
    cout << "add result = " << a.add() << endl;
}

//=========================================================
// template: 完全特化, 偏特化 例子

// 原始模板
template <typename T1, typename T2>
class MyClass {
public:
    void show() {
        cout << "General template\n";
    }
};

// 完全特化
template <>
class MyClass<int, double> {
public:
    void show() {
        cout << "Fully specialized for <int, double>\n";
    }
};

// 偏特化：当第二个参数是 int 时
template <typename T1>
class MyClass<T1, int> {
public:
    void show() {
        cout << "Partially specialized when T2 is int\n";
    }
};

void subtest2() {
    cout << __FUNCTION__ << endl;
    MyClass<char, float> a;  // 用通用模板
    a.show();
    MyClass<int, double> b;  // 用完全特化模板
    b.show();
    MyClass<float, int> c;  // 偏特化模板
    c.show();
}

//=========================================================
// template: 模板类继承

#include <iostream>
using namespace std;

// 模板基类
template <typename T>
class Base {
public:
    T value;

    Base(T v) : value(v) {}

    virtual void show() const {
        cout << "Base with value: " << value << endl;
    }

    virtual void show2(T extra) const {
        cout << "Base show2 extra =" << extra << endl;
    }

    virtual ~Base() = default;  // 虚析构函数，保证多态删除安全
};

// 模板派生类
template <typename T>
class Derived : public Base<T> {
public:
    Derived(T v) : Base<T>(v) {}

    void show() const override {
        cout << "Derived: " << this->value + 1 << endl;
    }

    // 虽然在模板类中定义虚函数并使用模板参数作为参数类型是合法的，
    // 但这可能会导致每个模板实例都有自己的一套虚函数表，从而增加代码体积。​
    // 因此，在设计时应权衡代码的灵活性与可维护性。
    void show2(T extra) const override {
        cout << "Derived show2, extra = " << extra << endl;
    }
};

// 派生类是非模板类
class DerivedInt : public Base<int> {
public:
    DerivedInt(int v) : Base<int>(v) {}

    void show() const override {
        cout << "DerivedInt: " << this->value + 2 << endl;
    }
};

void subtest3() {
    cout << __FUNCTION__ << endl;

    Base<int>* ptr = new Derived<int>(10);
    ptr->show();     // // 多态调用, 输出11
    ptr->show2(12);  // // 多态调用, 输出11
    delete ptr;      // 正确释放，因基类有虚析构

    Base<int>* ptr2 = new DerivedInt(10);
    ptr2->show();
    delete ptr2;
}

int cppMain() {
    subtest1();
    subtest2();
    subtest3();

    return 0;
}

}  // namespace template_basic

/*===== Output =====

[RUN  ] template_basic
subtest1
3, 3.3, Hello, World!
extra = 5, defaultExtra = 10
add result = 18.2
subtest2
General template
Fully specialized for <int, double>
Partially specialized when T2 is int
subtest3
Derived: 11
Derived show2, extra = 12
DerivedInt: 12
[tid=1] [Memory Report] globalNewCnt = 2, globalDeleteCnt = 2, globalNewMemSize = 120, globalDeleteMemSize = 120
[   OK] template_basic

*/
