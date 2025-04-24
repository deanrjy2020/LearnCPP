#include <cassert>
#include <iostream>
#include <mutex>

#define USE_MY_SHARED_PTR 1
#if !USE_MY_SHARED_PTR
#include <memory>  // uptr, sptr
#endif

using namespace std;

namespace impl_shared_ptr {

class A {
private:
    int a;

public:
    A(int x) : a(x) { printf("\t\tA con is called.\n"); }
    ~A() { printf("\t\tA des is called.\n"); }
    void printRes() { printf("\t\tprintRes func is called. a = %d.\n", a); }
};

#if USE_MY_SHARED_PTR

/*
1, 看这个前先看unique_ptr实现
2, 先不写mutex, 当做followup肯定问
    最后提到一个指针被重复去构建shared_ptr, double free, 用std的make_share() helper func.
    直接使用原始指针创建多个shared_ptr，从而导致多个独立的引用计数。
    解决方法,
    使用 make_shared 函数创建 shared_ptr实例，而不是直接使用原始指针。
    这样可以确保所有shared_ptr实例共享相同的引用计数。
3, bug 循环引用.
    https://csguide.cn/cpp/memory/shared_ptr.html#%E5%A6%82%E4%BD%95%E8%A7%A3%E5%86%B3-double-free
*/
template <typename T>
class shared_ptr {
private:
    T* rawPtr{nullptr};
    // 因为官方的use_count() API返回的是long, 所以这个count用long.
    // note, 从0开始.
    long* cnt{nullptr};
    // 用来保护cnt.
    mutex* mtx{nullptr};

    void decreaseCnt() {
        // corner case,
        // shared_ptr<A> sptr1(new A(1));
        // shared_ptr<A> sptr2; // default con, cnt为0
        // sptr2 = sptr1; // copy assignment op
        if (*cnt == 0) {
            assert(rawPtr == nullptr);
            return;
        }
        bool release = false;
        mtx->lock();
        if (--(*cnt) == 0) {
            delete rawPtr;
            delete cnt;
            rawPtr = nullptr;
            cnt = nullptr;

            release = true;
        }
        mtx->unlock();
        // 自己lock的时候当然不能delete自己, 用一个release flag.
        if (release) {
            delete mtx;
            mtx = nullptr;
        }
    }

public:
    // 额外说明noexcept
    // 所有的函数都在末尾(const后)加noexcept
    // noexcept, C++11关键字, 告诉编译器, 函数中不会发生异常.
    // 有利于编译器对程序做更多的优化,
    // C++中的异常处理是在运行时而不是编译时检测的,
    // 为了实现运行时检测，编译器创建额外的代码，然而这会妨碍程序优化

    // default and overloaded con
    // explicit构造函数是用来防止隐式转换, 即不允许写成shared_ptr<T> tempPtr = <T*>;
    explicit shared_ptr(T* p = nullptr) : rawPtr(p), cnt(new long(p ? 1 : 0)), mtx(new mutex) {
        printf("\tdefault & overloaded con is called.\n");
    }
    // copy con
    shared_ptr(const shared_ptr& other) {
        printf("\tcopy con is called.\n");
        // 不需要检测 this->rawPtr == other.rawPtr
        // 语法上不能自己初始化自己.

        // 初始化自身成员不用加锁, 只是把三个的地址告诉别人.
        // 也可以在初始化列表里面做.
        rawPtr = other.rawPtr;
        cnt = other.cnt;
        mtx = other.mtx;

        // 修改公共资源要锁.
        mtx->lock();
        (*cnt)++;
        mtx->unlock();
    }
    // copy assignment op
    shared_ptr& operator=(const shared_ptr& other) {
        printf("\tcopy assignment op is called.\n");
        // do nothing if self assignment.
        if (rawPtr != other.rawPtr) {
            // dereference cur one before referring to other.
            decreaseCnt();

            rawPtr = other.rawPtr;
            cnt = other.cnt;
            mtx = other.mtx;

            mtx->lock();
            (*cnt)++;
            mtx->unlock();
        }
        return *this;
    }

    // todo: add the move con here, same as the copy, but decrease the other afterwards.
    // 面试的时候先不加, 提忘了再加.

    ~shared_ptr() {
        printf("\tdes is called.\n");
        decreaseCnt();
    }

    // APIs
    T* operator->() const {
        printf("\t-> op is called.\n");
        return rawPtr;
    }
    T& operator*() const {
        printf("\t* op is called.\n");
        return *rawPtr;
    }
    T* get() { return rawPtr; }
    long use_count() const { return *cnt; }
    // no release() func
};
#endif

int cppMain() {
    A* rawPtr = new A(3);

    printf("test overloaded con.\n");
    shared_ptr<A> sptr0(nullptr);
    assert(sptr0.get() == nullptr);
    assert(sptr0.use_count() == 0);

    printf("test overloaded con.\n");
    shared_ptr<A> sptr1(rawPtr);
    assert(sptr1.get() == rawPtr);
    assert(sptr1.use_count() == 1);

    printf("test copy con 1.\n");
    shared_ptr<A> sptr2 = sptr1;
    printf("test copy con 2.\n");
    shared_ptr<A> sptr3(sptr2);
    assert(sptr3.get() == rawPtr);
    assert(sptr3.use_count() == 3);

    printf("test default con.\n");
    shared_ptr<A> sptr4;
    assert(sptr4.get() == nullptr);
    assert(sptr4.use_count() == 0);

    printf("test copy assignment op.\n");
    sptr4 = sptr1;
    assert(sptr4.get() == rawPtr);
    assert(sptr4.use_count() == 4);
    printf("test copy assignment op, self assigning.\n");
    sptr4 = sptr4;
    assert(sptr4.get() == rawPtr);
    assert(sptr4.use_count() == 4);

    return 0;
}

}  // namespace impl_shared_ptr

/*===== Output =====

[RUN  ] impl_shared_ptr
		A con is called.
test overloaded con.
	default & overloaded con is called.
test overloaded con.
	default & overloaded con is called.
test copy con 1.
	copy con is called.
test copy con 2.
	copy con is called.
test default con.
	default & overloaded con is called.
test copy assignment op.
	copy assignment op is called.
test copy assignment op, self assigning.
	copy assignment op is called.
	des is called.
	des is called.
	des is called.
	des is called.
		A des is called.
	des is called.
[   OK] impl_shared_ptr

*/
