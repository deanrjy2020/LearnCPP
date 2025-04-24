#include <cassert>
#include <iostream>

#define USE_MY_UNIQUE_PTR 1
#if !USE_MY_UNIQUE_PTR
#include <memory>  // uptr, sptr
#endif

using namespace std;

namespace impl_unique_ptr {

class A {
private:
    int a;

public:
    A(int x) : a(x) { printf("\t\tA con is called.\n"); }
    ~A() { printf("\t\tA des is called.\n"); }
    void printSth() { printf("\t\tprintSth func is called.\n"); }
    void printRes() { printf("\t\tprintRes func is called. a = %d.\n", a); }
};

#if USE_MY_UNIQUE_PTR
/*
1, 看这个前先看C++ constructor笔记, 各种constructor搞清楚
2, 类里面是用unique_ptr或者unique_ptr<T>都是可以的.
*/
template <typename T>
class unique_ptr {
private:
    // 类里面涉及到rawPtr的直接用copy, 似乎没必要用rawPtr=move(p);
    // 确保copy了后别人的是否要为nullptr (清空)
    T* rawPtr{nullptr};

public:
    // 额外说明noexcept
    // 所有的函数都在末尾(const后)加noexcept
    // noexcept, C++11关键字, 告诉编译器, 函数中不会发生异常.
    // 有利于编译器对程序做更多的优化, C++中的异常处理是在运行时而不是编译时检测的,
    // 为了实现运行时检测，编译器创建额外的代码，然而这会妨碍程序优化

    // default con
    // unique_ptr() : rawPtr(nullptr) {
    //     printf("\tdefault con is called.\n");
    // }
    // overloaded con
    // unique_ptr(nullptr_t p) : rawPtr(p) {
    //     // 这个con 1可以不用, 没有的话会call 下面这个overloaded con 2.
    //     printf("\toverloaded con 1 is called.\n");
    // }
    // explicit构造函数是用来防止隐式转换, 即不允许写成unique_ptr<T> tempPtr = <T*>;
    explicit unique_ptr(T* p = nullptr) : rawPtr(p) {
        printf("\tdefault & overloaded con is called.\n");
    }

    // copy con
    unique_ptr(const unique_ptr& other) = delete;
    // copy assignment op
    unique_ptr& operator=(const unique_ptr& other) = delete;

    // move con
    unique_ptr(unique_ptr&& other) {
        printf("\tmove con is called.\n");
        // 自己还没构建完成, 肯定是空的, 可以用swap, 后other就是空了.
        other.swap(*this);
    }
    // move assignment op
    // 和move con的思路一样, 多了一个自己=自己的情况, move con不存在这个情况.
    // 这里不能用other.swap(*this);本身可能有地址, 不是nullptr.
    unique_ptr& operator=(unique_ptr&& other) {
        printf("\tmove assignment op is called.\n");
        // 本身可能有地址, 不是nullptr, 先release, 否则会leak
        // 1, null = null
        // 2, null = ptr
        // 3, ptr = ptr, self assigning
        // 4, ptr = null
        if (rawPtr != other.rawPtr) {
            // ok to delete nullptr.
            delete rawPtr;
            rawPtr = nullptr;
        }
        other.swap(*this);
        return *this;
    }
    ~unique_ptr() {
        printf("\tdes is called.\n");
        delete rawPtr;
        rawPtr = nullptr;
    }

    // implement APIs
    T* operator->() const {
        printf("\t-> op is called.\n");
        return rawPtr;
    }
    T& operator*() const {
        printf("\t* op is called.\n");
        return *rawPtr;
    }
    T* get() {
        return rawPtr;
    }
    // releases the ownership of the resource.
    // The user is now responsible for memory clean-up.
    T* release() {
        auto tmp = rawPtr;
        rawPtr = nullptr;
        return tmp;
    }
    /*
    释放老资源, 指向新的, 参数为raw resource ptr
    测试考虑几种情况:
    1, 一般情况: uptr.reset(new A(5)); // ok
    1.1, reset成nullptr: uptr.reset(nullptr); // 应该ok
    2, reset成自己: uptr.reset(uptr.get()); // 应该ok
    3, reset成别人own的一个资源: uptr1.reset(uptr2.get());
        // user's error, 但是实现时候要检测到. 如何检测?
    4, reset成别人release的资源: uptr1.reset(uptr2.release()); // ok
    */
    void reset(T* p = nullptr) {
        // self-reset
        if (this->rawPtr == p) {
            return;
        }
        // delete nullptr is also ok.
        delete rawPtr;
        rawPtr = p;
    }
    void swap(unique_ptr& other) {
        std::swap(rawPtr, other.rawPtr);
    }
};
#endif

int cppMain() {
    A* rawPtr = new A(3);
    printf("test overloaded con 1.\n");
    unique_ptr<A> uptr0(nullptr);
    printf("test overloaded con 2.\n");
    unique_ptr<A> uptr1(rawPtr);
    printf("test -> op.\n");
    uptr1->printRes();
    printf("test * op.\n");
    (*uptr1).printRes();

    // unique_ptr<A> uptr2 = uptr1;  // copy con, compiler error
    // unique_ptr<A> uptr3(uptr1);   // copy con, compiler error
    printf("test default con.\n");
    unique_ptr<A> uptr4;
    // uptr4 = uptr1;  // copy assignment op, compiler error

    printf("test move con. 1\n");
    unique_ptr<A> uptr5(move(uptr1));
    printf("test move con. 2\n");
    unique_ptr<A> uptr6 = move(uptr5);

    printf("test move assignment op.\n");
    unique_ptr<A> uptr7;
    uptr7 = move(uptr6);  // move assignment op, ok

    // 在move assign op前, 8里面数据不会丢失.
    unique_ptr<A> uptr8(new A(4));
    uptr8 = move(uptr7);
    uptr7 = move(uptr8);

    uptr1->printSth();  // ok to use code, not the resource
    // compiler error, can't print the uptr value.
    // printf("test 2, uptr1 = 0x%x, \n", uptr1);
    printf("test resource value is correct.\n");
    uptr7->printRes();

    // unique_ptr API test
    assert(uptr4.get() == nullptr);
    uptr4.swap(uptr7);
    assert(rawPtr == uptr4.get());

    uptr7.reset(uptr4.release());
    assert(rawPtr == uptr7.get() && nullptr == uptr4.get());

    // uptr7.reset(uptr4.get()); // should error out

    // other test

    // comipiler err, can't call overloaded con since it is explicit
    // A* pa = new A(20);
    // unique_ptr<A> uptr20 = pa;
    // uptr20->printRes();

    return 0;
}

}  // namespace impl_unique_ptr

/*===== Output =====

[RUN  ] impl_unique_ptr
		A con is called.
test overloaded con 1.
	default & overloaded con is called.
test overloaded con 2.
	default & overloaded con is called.
test -> op.
	-> op is called.
		printRes func is called. a = 3.
test * op.
	* op is called.
		printRes func is called. a = 3.
test default con.
	default & overloaded con is called.
test move con. 1
	move con is called.
test move con. 2
	move con is called.
test move assignment op.
	default & overloaded con is called.
	move assignment op is called.
		A con is called.
	default & overloaded con is called.
	move assignment op is called.
		A des is called.
	move assignment op is called.
	-> op is called.
		printSth func is called.
test resource value is correct.
	-> op is called.
		printRes func is called. a = 3.
	des is called.
	des is called.
		A des is called.
	des is called.
	des is called.
	des is called.
	des is called.
	des is called.
[tid=1] [Memory Report] globalNewCnt = 2, globalDeleteCnt = 2, globalNewMemSize = 96, globalDeleteMemSize = 96
[   OK] impl_unique_ptr

*/
